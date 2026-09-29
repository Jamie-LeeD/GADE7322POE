#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DefenderSelectWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;
class UDefenderSelectWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefenderSelectClicked, UDefenderSelectWidget*, SelectWidget);

/**
 * Native parent of WBP_DefenderSelectBase: one clickable defender card.
 * It only tells the placement manager "the player selected this defender";
 * placement itself stays in BP_DefenderPlacementManager.
 */
UCLASS()
class GADE7322POE_API UDefenderSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Defender")
	void SetDefenderData(const FText& InName, int32 InPrice, TSubclassOf<AActor> InClass, UTexture2D* InImage);

	UFUNCTION(BlueprintCallable, Category = "Defender")
	void RefreshDisplay();

	/** Selects this defender (or cancels if it is already the selected one). */
	UFUNCTION(BlueprintCallable, Category = "Defender")
	void SelectThisDefender();

	/** DefenderPrice, or the class PlacementCost when DefenderPrice is 0. */
	UFUNCTION(BlueprintPure, Category = "Defender")
	int32 GetEffectivePrice() const;

	UFUNCTION(BlueprintPure, Category = "Defender")
	FText GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Defender")
	bool IsSelected() const;

	UPROPERTY(BlueprintAssignable, Category = "Defender")
	FOnDefenderSelectClicked OnDefenderClicked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender", meta = (ExposeOnSpawn = "true"))
	FText DefenderName;

	/** Gold cost charged on placement. 0 = use the defender class PlacementCost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender", meta = (ExposeOnSpawn = "true", ClampMin = "0"))
	int32 DefenderPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender", meta = (ExposeOnSpawn = "true"))
	TSubclassOf<AActor> DefenderClass;

	/** Portrait shown in the DefenderImage widget. Leave empty to show the placeholder colour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UTexture2D> DefenderIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender|Style")
	FLinearColor PlaceholderImageColor = FLinearColor(0.25f, 0.3f, 0.4f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender|Style")
	FLinearColor SelectedColor = FLinearColor(1.f, 0.78f, 0.15f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender|Style")
	FLinearColor HoverColor = FLinearColor(0.45f, 0.45f, 0.55f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender|Style")
	FLinearColor PriceColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender|Style")
	FLinearColor UnaffordablePriceColor = FLinearColor(1.f, 0.25f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender|Style", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UnavailableOpacity = 0.5f;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	void UpdateStateVisuals();
	UBorder* FindBackgroundBorder();

	UPROPERTY(BlueprintReadOnly, Category = "Defender|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefenderNameText;

	UPROPERTY(BlueprintReadOnly, Category = "Defender|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefenderPriceText;

	UPROPERTY(BlueprintReadOnly, Category = "Defender|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UImage> DefenderImage;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BackgroundBorder;

	FLinearColor NormalBorderColor = FLinearColor::White;
	bool bBorderColorCaptured = false;
	bool bHovered = false;
};
