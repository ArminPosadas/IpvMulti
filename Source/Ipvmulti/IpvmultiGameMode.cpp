
#include "IpvmultiGameMode.h"

#include "IpvmultiGameMode.h"
#include "IpvmultiCharacter.h"
#include "Game/IpvMultiGameState.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AIpvmultiGameMode::AIpvmultiGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	GameStateClass = AIpvMultiGameState::StaticClass();
}

void AIpvmultiGameMode::CompleteMission(APawn* Pawn)
{
	if (Pawn == nullptr) return;
	Pawn->DisableInput(nullptr);
	if (SpectatorViewClass)
	{
		TArray<AActor*> ReturnActors;
		UGameplayStatics::GetAllActorsOfClass(this, SpectatorViewClass, ReturnActors);
		if (ReturnActors.Num() > 0)
		{
			AActor* NewViewTarget = ReturnActors[0];
			for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator(); It; It++)
			{
				APlayerController*PC=It->Get();
				if (PC)
				{
					PC->SetViewTargetWithBlend(NewViewTarget, 1.0f, VTBlend_Cubic);
				}
			}
		}
	}
	AIpvMultiGameState* GS = GetGameState<AIpvMultiGameState>();
	if (GS)
	{
		GS->MultiCastOnMissionComplete(Pawn, true);
	}
	OnMissionCompleted(Pawn);
}