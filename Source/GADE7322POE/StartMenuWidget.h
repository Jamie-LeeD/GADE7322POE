#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartMenuWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartMenuPlayClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartMenuStrategyChosen, bool, bBudgetStrategy);

/**
 * Title screen: Play → Budget / Counter.
 * Prefer editing sizes on WBP_StartMenu Class Defaults (UI | Layout), or place Size Boxes in the WBP designer.
 */
UCLASS()
class GADE7322POE_API UStartMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable, Category = "Start Menu")
	FOnStartMenuPlayClicked OnPlayClicked;

	UPROPERTY(BlueprintAssignable, Category = "Start Menu")
	FOnStartMenuStrategyChosen OnStrategyChosen;

	/** Max box for the title logo (aspect preserved). Edit on WBP_StartMenu Class Defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D TitleMaxSize = FVector2D(980.f, 360.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D PlayButtonMaxSize = FVector2D(760.f, 260.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D StrategyPanelMaxSize = FVector2D(980.f, 860.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D StrategyButtonMaxSize = FVector2D(720.f, 280.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layout")
	FVector2D BackButtonMaxSize = FVector2D(600.f, 200.f);

protected:
	void BuildLayout();
	void BindButtonEvents();
	void ApplyArtStyles();
	void ScheduleRestyle();
	void ShowTitlePage();
	void ShowStrategyPage();

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleBudgetClicked();

	UFUNCTION()
	void HandleCounterClicked();

	UFUNCTION()
	void HandleBackClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UButton> BudgetButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UButton> CounterButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> TitleImage;

	/** Full-screen grey blank layer behind the start-screen background art. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> GreyBackdrop;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> StrategyPanel;

	bool bLayoutBuilt = false;
	bool bEventsBound = false;
};
