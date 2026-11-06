#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameHudWidget.generated.h"

class UBorder;
class UOverlay;
class UTextBlock;

/**
 * Parent for WBP_GameHUD. Styles the HUD overlay/panel with blankpanel2 art.
 * Bind optional widgets named PanelBorder / BackgroundBorder / HUDOverlay / GoldText / TowerHealthText.
 */
UCLASS()
class GADE7322POE_API UGameHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D PanelMaxSize = FVector2D(1600.f, 420.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Style")
	FLinearColor HudTextColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

protected:
	void ApplyPanelArt();
	void ApplyHudTextColors();
	void ScheduleRestyle();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UOverlay> HUDOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> TowerHealthText;

	float TextColorRefreshAt = 0.f;
};
