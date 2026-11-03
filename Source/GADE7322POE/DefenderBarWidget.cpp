#include "DefenderBarWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "DefenderPlacementManagerBase.h"
#include "DefenderSelectWidget.h"

bool UDefenderBarWidget::HasUsableDefenderEntries() const
{
	for (const FDefenderBarEntry& Entry : Defenders)
	{
		if (Entry.DefenderClass)
		{
			return true;
		}
	}
	return false;
}

void UDefenderBarWidget::EnsureDefaultDefenders()
{
	if (HasUsableDefenderEntries())
	{
		return;
	}

	UClass* RoyalArcher = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalArcher.BP_RoyalArcher_C"));
	UClass* FrostArcher = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_FrostArcher.BP_FrostArcher_C"));
	UClass* RoyalGuardian = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalGuardian.BP_RoyalGuardian_C"));

	Defenders.Reset();

	auto AddEntry = [this](const TCHAR* Name, UClass* Class, int32 UpgradeGold)
	{
		if (!Class)
		{
			return;
		}
		FDefenderBarEntry& Entry = Defenders.AddDefaulted_GetRef();
		Entry.DefenderName = FText::FromString(Name);
		Entry.DefenderPrice = 0; // Uses each Blueprint's PlacementCost
		Entry.DefenderClass = Class;
		Entry.UpgradeCost = UpgradeGold;
	};

	AddEntry(TEXT("Royal Archer"), RoyalArcher, 80);
	AddEntry(TEXT("Frost Archer"), FrostArcher, 100);
	AddEntry(TEXT("Royal Guardian"), RoyalGuardian, 120);
}

void UDefenderBarWidget::RebuildDefenderButtons()
{
	if (!DefenderBox)
	{
		return;
	}

	EnsureDefaultDefenders();

	for (UDefenderSelectWidget* Button : GeneratedButtons)
	{
		if (Button)
		{
			Button->RemoveFromParent();
		}
	}
	GeneratedButtons.Reset();

	// Keep hand-placed buttons only if they already have a defender class assigned.
	// Otherwise clear them and rebuild from the Defenders list (fixes wiped Blueprint data).
	TArray<UDefenderSelectWidget*> HandPlaced;
	for (UWidget* Child : DefenderBox->GetAllChildren())
	{
		if (UDefenderSelectWidget* Select = Cast<UDefenderSelectWidget>(Child))
		{
			HandPlaced.Add(Select);
		}
	}

	bool bHandPlacedUsable = HandPlaced.Num() > 0;
	for (const UDefenderSelectWidget* Select : HandPlaced)
	{
		if (!Select || !Select->DefenderClass)
		{
			bHandPlacedUsable = false;
			break;
		}
	}

	if (bHandPlacedUsable)
	{
		for (int32 Index = 0; Index < HandPlaced.Num(); ++Index)
		{
			UDefenderSelectWidget* Select = HandPlaced[Index];
			if (Defenders.IsValidIndex(Index) && Select->UpgradeCost <= 0)
			{
				Select->UpgradeCost = Defenders[Index].UpgradeCost;
			}
			Select->RefreshDisplay();
		}
		return;
	}

	for (UDefenderSelectWidget* Select : HandPlaced)
	{
		if (Select)
		{
			Select->RemoveFromParent();
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
		if (!Entry.DefenderClass)
		{
			continue;
		}
		UDefenderSelectWidget* Button = CreateWidget<UDefenderSelectWidget>(this, SelectClass);
		if (!Button)
		{
			continue;
		}
		Button->SetDefenderData(Entry.DefenderName, Entry.DefenderPrice, Entry.DefenderClass, Entry.DefenderIcon, Entry.UpgradeCost);
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
	RebuildDefenderButtons();
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
