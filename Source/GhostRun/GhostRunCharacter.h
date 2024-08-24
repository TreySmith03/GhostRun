// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GhostRunCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AGhostRunCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/* Dash Input Action*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DashStationaryAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DashMovingAction;

public:
	AGhostRunCharacter();
	

protected:

	/** Called for movement input */

	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for dashing input */
	void DashStationary(const FInputActionValue& Value);
	void DashMoving(const FInputActionValue& Value);
	void DashHandler(FVector dashDirection);
	void StopDashing();

	//Reset timer after dashDelay seconds
	void ResetTimerDashDelay();

	//Verify if character has been in a not falling state since last dash occurred
	void ResetMovementSkillsOnFloorContact();

	//Handle floor and wall jumps
	void JumpHandler();
	bool IsWallSliding();
	void WallJump();
	void SpringBoardJump();

	FHitResult ContactWithTerrainCheck(FVector direction, float distance);
	bool OnGround();
			

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	// Called every frame
	virtual void Tick(float DeltaTime);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:

	FVector2D MovementVector;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Dashing")
	float dashSpeed = 2000;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Dashing")
	float dashDelay = 1;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Dashing")
	float dashDuration = .25;

	bool canDash;
	bool isDashing;
	FTimerHandle dashResetTimerHandler;
	FTimerHandle dashDurationTimerHandler;

	bool timerComplete;

	bool hasContactedFloorSinceLastDash;
	bool hasContactedFloorSinceLastWallJump;

	TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Pawn;

	UPROPERTY(EditAnywhere, Category = "Character Movement")
	float maxContactFloorDistance = 100;

	UPROPERTY(EditAnywhere, Category = "Character Movement")
	float playerGravity = 2.0;
	
	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float jumpVelocityZ = 1000;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float maxWallClingDistance = 50;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float wallSlideInterpSpeed = 1500;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float wallSlideSpeed = 0;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float wallJumpVelocityY = 1200;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float wallJumpVelocityZ = 1000;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float springBoardJumpVelocityY = 2000;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Jumping / Falling")
	float springBoardJumpVelocityZ = 1000;
};

