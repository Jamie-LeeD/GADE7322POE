#include "DefenderSelectWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "DefenderPlacementManagerBase.h"
#include "Engine/Texture2D.h"

void UDefenderSelectWidget::SetDefenderData(const FText& InName, int32 InPrice, TSubclassOf<AActor> InClass, UTexture2D* InImage, int32 InUpgradeCost)
{
	DefenderName = InName;
	DefenderPrice = FMath::Max(0, InPrice);
	UpgradeCost = FMath::Max(0, InUpgradeCost);
	DefenderClass = InClass;
	DefenderIcon = InImage;
	RefreshDisplay();
}

int32 UDefenderSelectWidget::GetEffectivePrice() const
{
	return DefenderPrice > 0 ? DefenderPrice : ADefenderPlacementManagerBase::GetDefenderClassCost(DefenderClass, 0);
}

int32 UDefenderSelectWidget::GetEffectiveUpgradeCost() const
{
	return FMath::Max(0, UpgradeCost);
}

FText UDefenderSelectWidget::GetDisplayName() const
{
	if (!DefenderName.IsEmpty())
	{
		return DefenderName;
	}
	if (DefenderClass)
	{
		FString Name = DefenderClass->GetName();
		Name.RemoveFromStart(TEXT("BP_"));
		Name.RemoveFromEnd(TEXT("_C"));
		return FText::FromString(FName::NameToDisplayString(Name, false));
	}
	return FText::FromString(TEXT("Defender"));
}

FText UDefenderSelectWidget::GetUpgradeButtonLabel() const
{
	return FText::FromString(FString::Printf(TEXT("^^ %d ^^"), GetEffectiveUpgradeCost()));
}

bool UDefenderSelectWidget::IsSelected() const
{
	const ADefenderPlacementManagerBase* Manager = ADefenderPlacementManagerBase::GetDefenderPlacementManager(this);
	return Manager && Manager->bIsPlacingDefender && DefenderClass && Manager->SelectedDefenderClass == DefenderClass;
}

bool UDefenderSelectWidget::IsUpgradeModeSelected() const
{
	const ADefenderPlacementManagerBase* Manager = ADefenderPlacementManagerBase::GetDefenderPlacementManager(this);
	return Manager && Manager->bIsUpgradingDefender && DefenderClass && Manager->SelectedDefenderClass == DefenderClass;
}

void UDefenderSelectWidget::BindUpgradeButton()
{
	if (!Button_Upgrade)
	{
		return;
	}
	Button_Upgrade->OnClicked.RemoveDynamic(this, &UDefenderSelectWidget::HandleUpgradeButtonClicked);
	Button_Upgrade->OnClicked.AddDynamic(this, &UDefenderSelectWidget::HandleUpgradeButtonClicked);
}

void UDefenderSelectWidget::HandleUpgradeButtonClicked()
{
	BeginUpgradeForThisDefender();
}

void UDefenderSelectWidget::BeginUpgradeForThisDefender()
{
	ADefenderPlacementManagerBase* Manager = ADefenderPlacementManagerBase::GetDefenderPlacementManager(this);
	if (!Manager)
	{
		return;
	}
	if (IsUpgradeModeSelected())
	{
		Manager->CancelDefenderPlacement();
		return;
	}
	Manager->BeginUpgradeMode(DefenderClass, GetEffectiveUpgradeCost(), GetDisplayName());
}

UTextBlock* UDefenderSelectWidget::FindUpgradeButtonText() const
{
	if (CachedUpgradeButtonText)
	{
		return CachedUpgradeButtonText;
	}
	if (!Button_Upgrade)
	{
		return nullptr;
	}

	UWidget* Content = Button_Upgrade->GetContent();
	if (UTextBlock* DirectText = Cast<UTextBlock>(Content))
	{
		return DirectText;
	}
	if (UPanelWidget* Panel = Cast<UPanelWidget>(Content))
	{
		for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
		{
			if (UTextBlock* ChildText = Cast<UTextBlock>(Panel->GetChildAt(Index)))
			{
				return ChildText;
			}
		}
	}
	return nullptr;
}

void UDefenderSelectWidget::RefreshUpgradeButtonText()
{
	if (UTextBlock* UpgradeText = FindUpgradeButtonText())
	{
		CachedUpgradeButtonText = UpgradeText;
		UpgradeText->SetText(GetUpgradeButtonLabel());
	}
}

void UDefenderSelectWidget::RefreshDisplay()
{
	if (DefenderNameText)
	{
		DefenderNameText->SetText(GetDisplayName());
	}
	if (DefenderPriceText)
	{
		const int32 Price = GetEffectivePrice();
		DefenderPriceText->SetText(DefenderClass
			? FText::FromString(FString::Printf(TEXT("%d Gold"), Price))
			: FText::FromString(TEXT("Unavailable")));
	}
	if (DefenderImage)
	{
		if (DefenderIcon)
		{
			DefenderImage->SetBrushFromTexture(DefenderIcon, false);
			DefenderImage->SetColorAndOpacity(FLinearColor::White);
		}
		else if (!DefenderImage->GetBrush().GetResourceObject())
		{
			DefenderImage->SetColorAndOpacity(PlaceholderImageColor);
		}
	}
	RefreshUpgradeButtonText();
}

UBorder* UDefenderSelectWidget::FindBackgroundBorder()
{
	if (!BackgroundBorder && WidgetTree)
	{
		BackgroundBorder = Cast<UBorder>(GetRootWidget());
		if (!BackgroundBorder)
		{
			WidgetTree->ForEachWidget([this](UWidget* Widget)
			{
				if (!BackgroundBorder)
				{
					BackgroundBorder = Cast<UBorder>(Widget);
				}
			});
		}
	}
	if (BackgroundBorder && !bBorderColorCaptured)
	{
		NormalBorderColor = BackgroundBorder->GetBrushColor();
		bBorderColorCaptured = true;
	}
	return BackgroundBorder;
}

void UDefenderSelectWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
}

void UDefenderSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Must be hit-testable to receive clicks (UserWidgets default to self-hit-test-invisible).
	SetVisibility(ESlateVisibility::Visible);
	FindBackgroundBorder();
	BindUpgradeButton();
	RefreshDisplay();
	UpdateStateVisuals();
}

void UDefenderSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateStateVisuals();
}

void UDefenderSelectWidget::UpdateStateVisuals()
{
	const ADefenderPlacementManagerBase* Manager = ADefenderPlacementManagerBase::GetDefenderPlacementManager(this);
	const bool bSelected = IsSelected();
	const bool bUpgradeSelected = IsUpgradeModeSelected();
	const bool bAffordable = !Manager || Manager->CanAffordDefender(GetEffectivePrice());
	const bool bUpgradeAffordable = !Manager || Manager->CanAffordDefender(GetEffectiveUpgradeCost());
	const bool bAvailable = DefenderClass != nullptr;

	if (UBorder* Border = FindBackgroundBorder())
	{
		Border->SetBrushColor((bSelected || bUpgradeSelected) ? SelectedColor : (bHovered && bAvailable ? HoverColor : NormalBorderColor));
	}
	if (DefenderPriceText)
	{
		DefenderPriceText->SetColorAndOpacity(FSlateColor(bAffordable ? PriceColor : UnaffordablePriceColor));
	}
	if (UTextBlock* UpgradeText = FindUpgradeButtonText())
	{
		UpgradeText->SetColorAndOpacity(FSlateColor((bUpgradeAffordable || bUpgradeSelected) ? PriceColor : UnaffordablePriceColor));
	}
	if (Button_Upgrade)
	{
		Button_Upgrade->SetIsEnabled(bAvailable && (bUpgradeAffordable || bUpgradeSelected));
	}
	SetRenderOpacity((bAvailable && (bAffordable || bSelected || bUpgradeSelected)) ? 1.f : UnavailableOpacity);
	SetRenderScale((bSelected || bUpgradeSelected) ? FVector2D(1.06f, 1.06f) : FVector2D(1.f, 1.f));
}

void UDefenderSelectWidget::SelectThisDefender()
{
	OnDefenderClicked.Broadcast(this);

	ADefenderPlacementManagerBase* Manager = ADefenderPlacementManagerBase::GetDefenderPlacementManager(this);
	if (!Manager)
	{
		return;
	}
	if (IsSelected())
	{
		Manager->CancelDefenderPlacement();
		return;
	}
	Manager->SelectDefender(DefenderClass, GetEffectivePrice(), GetDisplayName());
}

FReply UDefenderSelectWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SelectThisDefender();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDefenderSelectWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = true;
}

void UDefenderSelectWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
}
