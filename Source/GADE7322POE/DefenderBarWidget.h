#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DefenderBarWidget.generated.h"

class UDefenderSelectWidget;
class UHorizontalBox;
class UTextBlock;
class UTexture2D;

USTRUCT(BlueprintType)
struct FDefenderBarEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender")
	FText DefenderName;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender", meta = (ClampMin = "0"))
	int32 DefenderPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender")
	TSubclassOf<AActor> DefenderClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender")
	TObjectPtr<UTexture2D> DefenderIcon = nullptr;
};


UCLASS()
class GADE7322POE_API UDefenderBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Defender Bar")
	void RebuildDefenderButtons();

	UFUNCTION(BlueprintCallable, Category = "Defender Bar")
	void ShowFeedback(const FText& Message, bool bIsError, float Duration = 2.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Bar")
	TArray<FDefenderBarEntry> Defenders;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Bar")
	TSubclassOf<UDefenderSelectWidget> DefenderSelectWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Bar")
	float DefenderSpacing = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Bar|Style")
	FLinearColor InfoColor = FLinearColor(0.6f, 0.9f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Bar|Style")
	FLinearColor ErrorColor = FLinearColor(1.f, 0.3f, 0.25f, 1.f);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandlePlacementMessage(const FText& Message, bool bIsError);

	void BindToManager();
	void UpdateHeading();

	UPROPERTY(BlueprintReadOnly, Category = "Defender Bar|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> DefenderBox;

	UPROPERTY(BlueprintReadOnly, Category = "Defender Bar|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeadingText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDefenderSelectWidget>> GeneratedButtons;

	TWeakObjectPtr<class ADefenderPlacementManagerBase> BoundManager;
	FText DefaultHeading;
	FSlateColor DefaultHeadingColor;
	bool bHeadingCaptured = false;
	FText FeedbackText;
	bool bFeedbackIsError = false;
	float FeedbackTimeRemaining = 0.f;
};
