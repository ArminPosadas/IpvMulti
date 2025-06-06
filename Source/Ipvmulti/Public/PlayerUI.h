#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerUI.generated.h"

UCLASS()
class IPVMULTI_API UPlayerUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateAmmoCount(int32 CurrentAmmo, int32 MaxAmmo);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateHealth(float CurrentHealth, float MaxHealth);
};