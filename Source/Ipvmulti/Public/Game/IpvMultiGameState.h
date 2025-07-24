// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "IpvMultiGameState.generated.h"

/**
 * 
 */
UCLASS()
class IPVMULTI_API AIpvMultiGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	UFUNCTION(NetMulticast, Reliable)
	void MultiCastOnMissionComplete(APawn* InstigatorPawn, bool bMissionSuccess);
	
};
