// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaunchPad.generated.h"

class UBoxComponent;

UCLASS()
class IPVMULTI_API ALaunchPad : public AActor
{
	GENERATED_BODY()

public:
	ALaunchPad();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	UBoxComponent* OverlapComp;

	UPROPERTY(EditAnywhere, Category = "Components")
	float LaunchForce;

	UPROPERTY(EditAnywhere, Category = "Components")
	float LaunchAngle;
	
	UFUNCTION()
	void OverlapLaunchpad(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

public:
	virtual void Tick(float DeltaTime) override;
};
