#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameInfoWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class USizeBox;
class UTextBlock;

/** Pause overlay. Prefer sizing via WBP_GameInfo Class Defaults (UI | Layout) or designer Size Boxes. */
UCLASS()
class GADE7322POE_API UGameInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Info", meta = (WorldContext = "WorldContextObject"))
	static UGameInfoWidget* OpenInfoPanel(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "UI|Info", meta = (WorldContext = "WorldContextObject"))
	static bool IsInfoPanelOpen(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "UI|Info")
	void CloseAndResume();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D PanelMaxSize = FVector2D(1320.f, 940.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D CloseButtonMaxSize = FVector2D(150.f, 150.f);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BuildLayout();
	void BindEvents();
	void ApplyArtStyles();
	void ScheduleRestyle();
	void ApplyPausedState(bool bPaused);
	void AddSection(UScrollBox* Scroll, const FString& Title, const FString& Body);

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UScrollBox> InfoScroll;

	bool bLayoutBuilt = false;
	bool bEventsBound = false;
	bool bDidPause = false;
};
