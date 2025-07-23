// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IpvmultiPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class IPVMULTI_API AIpvmultiPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnMissionCompleted(APawn* IntigatorPawn, bool bIsMissionSuccess);
};
