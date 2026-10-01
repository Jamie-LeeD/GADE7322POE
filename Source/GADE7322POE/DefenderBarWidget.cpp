#include "DefenderBarWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "DefenderPlacementManagerBase.h"
#include "DefenderSelectWidget.h"

void UDefenderBarWidget::RebuildDefenderButtons()
{
	if (!DefenderBox)
	{
		return;
	}

	for (UDefenderSelectWidget* Button : GeneratedButtons)
	{
		if (Button)
		{
			Button->RemoveFromParent();
		}
	}
	GeneratedButtons.Reset();

	// Hand-placed select widgets in the designer take priority over the Defenders list.
	for (UWidget* Child : DefenderBox->GetAllChildren())
	{
		if (Cast<UDefenderSelectWidget>(Child))
		{
			return;
		}
	}

	UClass* SelectClass = DefenderSelectWidgetClass.Get();
	if (!SelectClass)
	{
		SelectClass = StaticLoadClass(UDefenderSelectWidget::StaticClass(), nullptr, TEXT("/Game/UI/Bases/WBP_DefenderSelectBase.WBP_DefenderSelectBase_C"));
	}
	if (!SelectClass)
	{
		return;
	}

	for (const FDefenderBarEntry& Entry : Defenders)
	{
		UDefenderSelectWidget* Button = CreateWidget<UDefenderSelectWidget>(this, SelectClass);
		if (!Button)
		{
			continue;
		}
		Button->SetDefenderData(Entry.DefenderName, Entry.DefenderPrice, Entry.DefenderClass, Entry.DefenderIcon);
		if (UHorizontalBoxSlot* BoxSlot = DefenderBox->AddChildToHorizontalBox(Button))
		{
			BoxSlot->SetPadding(FMargin(DefenderSpacing * 0.5f, 0.f));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
			BoxSlot->SetHorizontalAlignment(HAlign_Center);
		}
		GeneratedButtons.Add(Button);
	}
}

void UDefenderBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RebuildDefenderButtons();
}

void UDefenderBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (HeadingText && !bHeadingCaptured)
	{
		DefaultHeading = HeadingText->GetText();
		DefaultHeadingColor = HeadingText->GetColorAndOpacity();
		bHeadingCaptured = true;
	}
	if (GeneratedButtons.Num() == 0)
	{
		RebuildDefenderButtons();
	}
	BindToManager();
}

void UDefenderBarWidget::NativeDestruct()
{
	if (ADefenderPlacementManagerBase* Manager = BoundManager.Get())
	{
		Manager->OnDefenderPlacementMessage.RemoveDynamic(this, &UDefenderBarWidget::HandlePlacementMessage);
	}
	BoundManager.Reset();
	Super::NativeDestruct();
}

void UDefenderBarWidget::BindToManager()
{
	if (BoundManager.IsValid())
	{
		return;
	}
	if (ADefenderPlacementManagerBase* Manager = ADefenderPlacementManagerBase::GetDefenderPlacementManager(this))
	{
		Manager->OnDefenderPlacementMessage.AddUniqueDynamic(this, &UDefenderBarWidget::HandlePlacementMessage);
		BoundManager = Manager;
	}
}

void UDefenderBarWidget::HandlePlacementMessage(const FText& Message, bool bIsError)
{
	ShowFeedback(Message, bIsError);
}

void UDefenderBarWidget::ShowFeedback(const FText& Message, bool bIsError, float Duration)
{
	FeedbackText = Message;
	bFeedbackIsError = bIsError;
	FeedbackTimeRemaining = Duration;
	UpdateHeading();
}

void UDefenderBarWidget::UpdateHeading()
{
	if (!HeadingText)
	{
		return;
	}

	const ADefenderPlacementManagerBase* Manager = BoundManager.Get();
	if (FeedbackTimeRemaining > 0.f)
	{
		HeadingText->SetText(FeedbackText);
		HeadingText->SetColorAndOpacity(FSlateColor(bFeedbackIsError ? ErrorColor : InfoColor));
	}
	else if (Manager && Manager->bIsPlacingDefender)
	{
		HeadingText->SetText(FText::FromString(FString::Printf(TEXT("Placing %s (%d Gold): click a free pad  -  Right-click to cancel"),
			*Manager->SelectedDefenderName.ToString(), Manager->SelectedDefenderCost)));
		HeadingText->SetColorAndOpacity(FSlateColor(InfoColor));
	}
	else if (bHeadingCaptured)
	{
		HeadingText->SetText(DefaultHeading);
		HeadingText->SetColorAndOpacity(DefaultHeadingColor);
	}
}

void UDefenderBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	BindToManager();
	if (FeedbackTimeRemaining > 0.f)
	{
		FeedbackTimeRemaining -= InDeltaTime;
	}
	UpdateHeading();

	// Only show the bar while the match is actually being played (hidden on game over etc.).
	if (UWidget* Root = GetRootWidget())
	{
		const ADefenderPlacementManagerBase* Manager = BoundManager.Get();
		const bool bShow = Manager && Manager->IsGameplayActive();
		const ESlateVisibility Wanted = bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		if (Root->GetVisibility() != Wanted)
		{
			Root->SetVisibility(Wanted);
		}
	}
}

FReply UDefenderBarWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Clicks on the bar background must not fall through to the world (placement clicks).
	return FReply::Handled();
}
