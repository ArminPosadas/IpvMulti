#include "Game/ipvmultiGameState.h"

#include "EngineUtils.h"
#include "Player/IpvmultiPlayerController.h"

void AIpvMultiGameState::MultiCastOnMissionCompleted_Implementation(APawn* instigatorPawn, bool bMissionSuccess)
{

	/*if (APlayerController PC = GetWorld()->GetFirstPlayerController())
		if (APawn* Pawn = PC->GetPawn())
			Pawn->DisableInput(nullptr);*/

	for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		AIpvmultiPlayerController* PC=Cast<AIpvmultiPlayerController>(It->Get());

		if (PC)
		{
			PC->OnMissionCompleted(instigatorPawn, bMissionSuccess);
			APawn* Pawn =PC->GetPawn();
			if (Pawn)
			{
				Pawn->DisableInput(nullptr);
			}
		}
	}
}