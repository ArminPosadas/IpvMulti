#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyAmmoWidget.generated.h"

UCLASS()
class IPVMULTI_API UMyAmmoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateAmmoCount(int32 CurrentAmmo, int32 MaxAmmo);
};
