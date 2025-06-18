#include "AmmoPickup.h"
#include "D:\Github\IpvMulti\Source\Ipvmulti\IpvmultiCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(50.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AAmmoPickup::OnOverlapBegin);
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
}

void AAmmoPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AIpvmultiCharacter* Player = Cast<AIpvmultiCharacter>(OtherActor))
	{
		Player->AddAmmo(AmmoAmount);
		Destroy();
	}
}