// Copyright Epic Games, Inc. All Rights Reserved.

#include "GhostRunGameMode.h"
#include "GhostRunCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGhostRunGameMode::AGhostRunGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
