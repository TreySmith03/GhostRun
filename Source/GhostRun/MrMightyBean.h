// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MrMightyBean.generated.h"

UCLASS()
class GHOSTRUN_API AMrMightyBean : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMrMightyBean();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	UFUNCTION(BlueprintCallable)
	void Jumper();
};
