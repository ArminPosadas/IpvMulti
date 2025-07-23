#include "EngineUtils.h"
#include "Game/IpvMultiGameState.h"

void AIpvMultiGameState::MulticastOnMissionComplete_Implementation(APawn* InstigatorPawn, bool bMissionSuccess)
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->DisableInput(nullptr);
		}
	}
}
