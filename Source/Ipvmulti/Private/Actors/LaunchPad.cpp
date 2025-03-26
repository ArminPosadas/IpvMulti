// Fill out your copyright notice in the Description page of Project Settings.


#include "Ipvmulti/Public/Actors/LaunchPad.h"

#include "Components/BoxComponent.h"


// Sets default values
ALaunchPad::ALaunchPad()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>("MeshComp");
	OverlapComp = CreateDefaultSubobject<UBoxComponent>("BoxComp");
	RootComponent = OverlapComp;
	MeshComp->SetupAttachment(OverlapComp);
}

// Called when the game starts or when spawned
void ALaunchPad::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALaunchPad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

