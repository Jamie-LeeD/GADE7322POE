#include "HealthBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"

void UHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureWidgets();
}

TSharedRef<SWidget> UHealthBarWidget::RebuildWidget()
{
	EnsureWidgets();
	return Super::RebuildWidget();
}

void UHealthBarWidget::EnsureWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox"));
		Size->SetWidthOverride(120.f);
		Size->SetHeightOverride(14.f);
		Root->AddChildToVerticalBox(Size);

		HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
		HealthBar->SetPercent(1.f);
		HealthBar->SetFillColorAndOpacity(FLinearColor(0.15f, 0.85f, 0.2f, 1.f));
		Size->SetContent(HealthBar);

		HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
		HealthText->SetVisibility(bShowNumericText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		FSlateFontInfo Font = HealthText->GetFont();
		Font.Size = 10;
		HealthText->SetFont(Font);
		HealthText->SetJustification(ETextJustify::Center);
		Root->AddChildToVerticalBox(HealthText);
	}
	else if (!HealthBar)
	{
		HealthBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("HealthBar")));
		HealthText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HealthText")));
	}
}

void UHealthBarWidget::SetShowNumericText(bool bInShowNumericText)
{
	bShowNumericText = bInShowNumericText;
	EnsureWidgets();
	if (HealthText)
	{
		HealthText->SetVisibility(bShowNumericText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UHealthBarWidget::SetHealth(float Current, float Max)
{
	EnsureWidgets();
	const float SafeMax = FMath::Max(Max, KINDA_SMALL_NUMBER);
	const float Percent = FMath::Clamp(Current / SafeMax, 0.f, 1.f);
	SetHealthPercent(Percent);

	if (HealthText && bShowNumericText)
	{
		HealthText->SetVisibility(ESlateVisibility::HitTestInvisible);
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Current, Max)));
	}
}

void UHealthBarWidget::SetHealthPercent(float Percent)
{
	EnsureWidgets();
	Percent = FMath::Clamp(Percent, 0.f, 1.f);
	if (HealthBar)
	{
		HealthBar->SetPercent(Percent);
		ApplyFillColor(Percent);
	}
}

void UHealthBarWidget::ApplyFillColor(float Percent)
{
	if (!HealthBar)
	{
		return;
	}

	FLinearColor Color;
	if (Percent > 0.55f)
	{
		Color = FLinearColor(0.15f, 0.85f, 0.2f, 1.f);
	}
	else if (Percent > 0.25f)
	{
		Color = FLinearColor(0.95f, 0.75f, 0.1f, 1.f);
	}
	else
	{
		Color = FLinearColor(0.9f, 0.15f, 0.1f, 1.f);
	}
	HealthBar->SetFillColorAndOpacity(Color);
}
