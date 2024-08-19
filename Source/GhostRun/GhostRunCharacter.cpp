// Copyright Epic Games, Inc. All Rights Reserved.

#include "GhostRunCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AGhostRunCharacter

AGhostRunCharacter::AGhostRunCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Configure for dashes
	canDash = true;
	timerComplete = true;
	hasContactedFloorSinceLastDash = true;

	//Configure Tick
	PrimaryActorTick.bCanEverTick = true;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AGhostRunCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void AGhostRunCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Check contact with floor to reset dash
	CheckHasContactedFloorSinceLastDash();

	IsWallSliding();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGhostRunCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGhostRunCharacter::JumpHandler);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGhostRunCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGhostRunCharacter::Look);

		// Dashing
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AGhostRunCharacter::Dash);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AGhostRunCharacter::JumpHandler()
{
	if(IsWallSliding())
	{
		WallJump();
	}
	else
	{
		ACharacter::Jump();
	}
}

void AGhostRunCharacter::WallJump()
{
	//Turn away from wall
	FRotator currentRotation = GetActorRotation();
	currentRotation.Yaw += 180;
	SetActorRotation(currentRotation);

	//launch character off of wall
	FVector forwardDir = GetActorForwardVector();
	forwardDir.GetSafeNormal(1);
	forwardDir.Normalize(1);
	forwardDir.X = 0;
	forwardDir.Y *= wallJumpVelocityY;
	forwardDir.Z = wallJumpVelocityZ;
	LaunchCharacter(forwardDir, true, true);
}

bool AGhostRunCharacter::IsWallSliding()
{
	if(GetCharacterMovement()->IsFalling())
	{
		// FHitResult will hold all data returned by our line collision query
		FHitResult Hit;

		// We set up a line trace from our current location to a point 1000cm ahead of us
		FVector TraceStart = GetActorLocation();
		FVector TraceEnd = GetActorLocation() + GetActorForwardVector() * maxWallClingDistance;

		// You can use FCollisionQueryParams to further configure the query
		// Here we add ourselves to the ignored list so we won't block the trace
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		// To run the query, you need a pointer to the current level, which you can get from an Actor with GetWorld()
		// UWorld()->LineTraceSingleByChannel runs a line trace and returns the first actor hit over the provided collision channel.
		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannelProperty, QueryParams);

		// You can use DrawDebug helpers and the log to help visualize and debug your trace queries.
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, Hit.bBlockingHit ? FColor::Blue : FColor::Red, false, 5.0f, 0, 10.0f);
		UE_LOG(LogTemp, Log, TEXT("Tracing line: %s to %s"), *TraceStart.ToCompactString(), *TraceEnd.ToCompactString());

		// If the trace hit something, bBlockingHit will be true,
		// and its fields will be filled with detailed info about what was hit
		if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
		{
			UE_LOG(LogTemp, Log, TEXT("Trace hit actor: %s"), *Hit.GetActor()->GetName());

			//Only slow character and face wall if character is descending
			if(GetVelocity().Z <= 0)
			{
				//Set character to face wall
				FRotator currentRotation = GetActorRotation();
				currentRotation.Yaw = Hit.ImpactNormal.Rotation().Yaw + 180;
				SetActorRotation(currentRotation);

				//Set character wall slide speed
				FVector currentVelocity = GetVelocity();
				FVector targetVelocity (currentVelocity.X, currentVelocity.Y, wallSlideSpeed);
				float DeltaTime = GetWorld()->GetDeltaSeconds();
				FVector newVelocity = FMath::VInterpConstantTo(currentVelocity, targetVelocity, DeltaTime, wallSlideInterpSpeed);
				
				UMovementComponent* MovementComp = GetMovementComponent();
				if (MovementComp)
				{
					MovementComp->Velocity = newVelocity;
				}
			}

			return true;
		}
		else 
		{
			UE_LOG(LogTemp, Log, TEXT("No Actors were hit"));
			return false;
		}
	}
	else
	{
		return false;
	}
}

void AGhostRunCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		//const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		//AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

//Look disabled to keep static camera
void AGhostRunCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	// FVector2D LookAxisVector = Value.Get<FVector2D>();

	// if (Controller != nullptr)
	// {
	// 	// add yaw and pitch input to controller
	// 	AddControllerYawInput(LookAxisVector.X);
	// 	AddControllerPitchInput(LookAxisVector.Y);
	// }
}

void AGhostRunCharacter::Dash(const FInputActionValue& Value)
{
	if(timerComplete && hasContactedFloorSinceLastDash)
	{
		//Add dash movement to character
		FVector forwardDir = GetVelocity();
		forwardDir.X = 0;
		forwardDir.Z = 0;
		forwardDir.GetSafeNormal(1);
		forwardDir.Normalize(1);
		LaunchCharacter(forwardDir * dashSpeed, true, true);

		//Dash won't be able to reset unless has touched the floor
		hasContactedFloorSinceLastDash = false;

		//Setup Timer
		timerComplete = false;
		GetWorld()->GetTimerManager().SetTimer(dashHandler, this, &AGhostRunCharacter::ResetTimerDashDelay, dashDelay, false);
	}
	
}

void AGhostRunCharacter::ResetTimerDashDelay()
{
	timerComplete = true;
}

void AGhostRunCharacter::CheckHasContactedFloorSinceLastDash()
{
	if(!GetCharacterMovement()->IsFalling())
		hasContactedFloorSinceLastDash = true;
}
