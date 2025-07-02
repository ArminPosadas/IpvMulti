// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ObjectiveActor.h"

#include "Components/SphereComponent.h"
#include "Ipvmulti/IpvmultiCharacter.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AObjectiveActor::AObjectiveActor()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = MeshComp;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AObjectiveActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AObjectiveActor::PlayerEffects()
{
	UGameplayStatics::SpawnEmitterAtLocation(this, PickupVFX, GetActorLocation());
}

void AObjectiveActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	PlayerEffects();
	AIpvmultiCharacter* myCharacter = Cast<AIpvmultiCharacter>(OtherActor);
	if (myCharacter)
	{
		myCharacter->bIsCarryingObjective = true;
		Destroy();
	}
}


