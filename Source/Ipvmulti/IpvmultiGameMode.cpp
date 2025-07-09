// Copyright Epic Games, Inc. All Rights Reserved.

#include "IpvmultiGameMode.h"

#include "EditorCategoryUtils.h"
#include "IpvmultiCharacter.h"
#include "UObject/ConstructorHelpers.h"

AIpvmultiGameMode::AIpvmultiGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AIpvmultiGameMode::CompleteMission(APawn* Pawn)
{
	if (Pawn == nullptr) return;
	{
		Pawn->DisableInput(nullptr);
		OnMissionCompleted(Pawn);
	}
}