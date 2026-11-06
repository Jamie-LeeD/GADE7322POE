#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OrnateMenuWidget.generated.h"

class UBorder;
class UOverlay;

/**
 * Shared parent for WBP_PauseMenu / WBP_GameOver.
 * Styles the background panel with /Game/UI/Art/pausepanel.
 */
UCLASS()
class GADE7322POE_API UOrnateMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D PanelMaxSize = FVector2D(720.f, 980.f);

protected:
	void ApplyPanelArt();
	void ScheduleRestyle();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UOverlay> PanelOverlay;
};
