#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class USizeBox;

/**
 * Reusable world-space health bar. Driven by UHealthBarComponent from any actor
 * that has a health component with CurrentHealth / MaxHealth.
 */
UCLASS()
class GADE7322POE_API UHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealth(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealthPercent(float Percent);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetShowNumericText(bool bInShowNumericText);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bShowNumericText = false;

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	TObjectPtr<UProgressBar> HealthBar = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	TObjectPtr<UTextBlock> HealthText = nullptr;

	void EnsureWidgets();
	void ApplyFillColor(float Percent);
};
