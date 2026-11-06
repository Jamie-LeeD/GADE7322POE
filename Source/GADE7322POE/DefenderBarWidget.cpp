#include "DefenderBarWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "DefenderPlacementManagerBase.h"
#include "DefenderSelectWidget.h"
#include "Engine/World.h"
#include "GameInfoWidget.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "UiArtLibrary.h"

namespace DefenderBarPrivate
{
	static void StyleBlankPanel2(UBorder* Border, FVector2D MaxSize)
	{
		if (!Border)
		{
			return;
		}
		UiArt::StyleBorderImage(Border, TEXT("blankpanel2"), MaxSize);
		FSlateBrush Brush = Border->Background;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		if (Brush.GetResourceObject())
		{
			Brush.Margin = FMargin(0.35f);
		}
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Border->SetBrush(Brush);
		Border->SetBrushColor(FLinearColor::White);
		Border->SetPadding(FMargin(16.f, 12.f));
	}

	static UBorder* FindFirstBorder(UWidget* Widget)
	{
		if (!Widget)
		{
			return nullptr;
		}
		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			return Border;
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			const int32 Num = Panel->GetChildrenCount();
			for (int32 Index = 0; Index < Num; ++Index)
			{
				if (UBorder* Found = FindFirstBorder(Panel->GetChildAt(Index)))
				{
					return Found;
				}
			}
		}
		return nullptr;
	}
}

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
	if (!HasUsableDefenderEntries())
	{
		UClass* RoyalArcher = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalArcher.BP_RoyalArcher_C"));
		UClass* FrostArcher = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_FrostArcher.BP_FrostArcher_C"));
		UClass* RoyalGuardian = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalGuardian.BP_RoyalGuardian_C"));

		Defenders.Reset();

		auto AddEntry = [this](const TCHAR* Name, UClass* Class, int32 PlaceGold, int32 UpgradeGold)
		{
			if (!Class)
			{
				return;
			}
			FDefenderBarEntry& Entry = Defenders.AddDefaulted_GetRef();
			Entry.DefenderName = FText::FromString(Name);
			// PlaceGold > 0 overrides Blueprint PlacementCost; 0 keeps the BP value.
			Entry.DefenderPrice = PlaceGold;
			Entry.DefenderClass = Class;
			Entry.UpgradeCost = UpgradeGold;
		};

		AddEntry(TEXT("Royal Archer"), RoyalArcher, 10, 80);
		AddEntry(TEXT("Frost Archer"), FrostArcher, 0, 100);
		AddEntry(TEXT("Royal Guardian"), RoyalGuardian, 0, 120);
	}

	// Always keep Royal Archer at 10 gold (overrides Blueprint PlacementCost / WBP defaults).
	for (FDefenderBarEntry& Entry : Defenders)
	{
		if (Entry.DefenderClass && Entry.DefenderClass->GetName().Contains(TEXT("RoyalArcher")))
		{
			Entry.DefenderPrice = 10;
		}
	}
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
			if (Defenders.IsValidIndex(Index))
			{
				if (Select->UpgradeCost <= 0)
				{
					Select->UpgradeCost = Defenders[Index].UpgradeCost;
				}
				if (Defenders[Index].DefenderPrice > 0)
				{
					Select->DefenderPrice = Defenders[Index].DefenderPrice;
				}
			}
			Select->RefreshDisplay();
		}
		EnsureInfoButton();
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

	EnsureInfoButton();
}

void UDefenderBarWidget::ApplyInfoButtonStyle()
{
	if (!InfoButton)
	{
		return;
	}
	UiArt::PreloadCommon();
	UiArt::StyleImageButton(InfoButton, TEXT("infobttn"), InfoButtonMaxSize);

	// Force parent Size Box to the layout max so a first-frame texture miss cannot stick tiny.
	if (USizeBox* Box = Cast<USizeBox>(InfoButton->GetParent()))
	{
		const FVector2D Fitted = UiArt::GetFittedSize(TEXT("infobttn"), InfoButtonMaxSize);
		const FVector2D Size = (Fitted.X >= 32.f && Fitted.Y >= 32.f) ? Fitted : InfoButtonMaxSize;
		Box->SetWidthOverride(Size.X);
		Box->SetHeightOverride(Size.Y);
	}
}

void UDefenderBarWidget::ScheduleInfoButtonRestyle()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UDefenderBarWidget::ApplyInfoButtonStyle));
	}
}

void UDefenderBarWidget::EnsureInfoButton()
{
	// Designer-placed InfoButton on WBP_DefenderBarBase
	if (InfoButton)
	{
		ApplyInfoButtonStyle();
		InfoButton->OnClicked.AddUniqueDynamic(this, &UDefenderBarWidget::HandleInfoClicked);
		return;
	}

	if (!DefenderBox)
	{
		return;
	}

	InfoButton = NewObject<UButton>(this, TEXT("InfoButton"));
	ApplyInfoButtonStyle();
	if (!UiArt::Load(TEXT("infobttn")))
	{
		UTextBlock* Label = NewObject<UTextBlock>(this, TEXT("InfoButtonLabel"));
		Label->SetText(FText::FromString(TEXT("Info")));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 16;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetJustification(ETextJustify::Center);
		InfoButton->AddChild(Label);
		FButtonStyle Style = InfoButton->GetStyle();
		Style.Normal.TintColor = FSlateColor(FLinearColor(0.15f, 0.28f, 0.42f, 0.95f));
		Style.Hovered.TintColor = FSlateColor(FLinearColor(0.25f, 0.45f, 0.65f, 1.f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.1f, 0.18f, 0.28f, 1.f));
		InfoButton->SetStyle(Style);
	}
	InfoButton->OnClicked.AddUniqueDynamic(this, &UDefenderBarWidget::HandleInfoClicked);

	if (UHorizontalBoxSlot* BoxSlot = DefenderBox->AddChildToHorizontalBox(InfoButton))
	{
		BoxSlot->SetPadding(FMargin(DefenderSpacing * 1.5f, 0.f, DefenderSpacing * 0.5f, 0.f));
		BoxSlot->SetVerticalAlignment(VAlign_Center);
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UDefenderBarWidget::HandleInfoClicked()
{
	UGameInfoWidget::OpenInfoPanel(this);
}

void UDefenderBarWidget::ApplyPanelArt()
{
	UiArt::PreloadCommon();

	UBorder* Target = PanelBorder ? PanelBorder.Get() : BackgroundBorder.Get();
	if (!Target && PanelOverlay)
	{
		Target = DefenderBarPrivate::FindFirstBorder(PanelOverlay);
	}
	if (!Target)
	{
		Target = DefenderBarPrivate::FindFirstBorder(GetRootWidget());
	}
	if (Target)
	{
		DefenderBarPrivate::StyleBlankPanel2(Target, PanelMaxSize);
	}
}

void UDefenderBarWidget::SchedulePanelRestyle()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UDefenderBarWidget::ApplyPanelArt));
	}
}

void UDefenderBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RebuildDefenderButtons();
	ApplyPanelArt();
}

void UDefenderBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UiArt::PreloadCommon();
	if (HeadingText && !bHeadingCaptured)
	{
		DefaultHeading = HeadingText->GetText();
		DefaultHeadingColor = HeadingText->GetColorAndOpacity();
		bHeadingCaptured = true;
	}
	RebuildDefenderButtons();
	ApplyPanelArt();
	SchedulePanelRestyle();
	ScheduleInfoButtonRestyle();
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
	else if (Manager && Manager->bIsUpgradingDefender)
	{
		HeadingText->SetText(FText::FromString(FString::Printf(TEXT("Upgrading %s (^^ %d ^^): click a placed defender  -  Right-click to cancel"),
			*Manager->SelectedDefenderName.ToString(), Manager->SelectedUpgradeCost)));
		HeadingText->SetColorAndOpacity(FSlateColor(InfoColor));
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
