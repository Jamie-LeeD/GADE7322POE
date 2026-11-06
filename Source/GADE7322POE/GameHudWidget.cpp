#include "GameHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "UiArtLibrary.h"

namespace GameHudPrivate
{
	static void StyleBorderBlankPanel2(UBorder* Border, FVector2D MaxSize)
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
		Border->SetPadding(FMargin(18.f, 14.f));
	}

	static void StyleBordersUnder(UWidget* Widget, FVector2D MaxSize, int32& StyledCount, int32 MaxToStyle)
	{
		if (!Widget || StyledCount >= MaxToStyle)
		{
			return;
		}
		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			StyleBorderBlankPanel2(Border, MaxSize);
			++StyledCount;
			return;
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			const int32 Num = Panel->GetChildrenCount();
			for (int32 Index = 0; Index < Num && StyledCount < MaxToStyle; ++Index)
			{
				StyleBordersUnder(Panel->GetChildAt(Index), MaxSize, StyledCount, MaxToStyle);
			}
		}
	}

	static void ForceWhiteText(UTextBlock* Text, const FLinearColor& Color)
	{
		if (!Text)
		{
			return;
		}
		Text->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UGameHudWidget::ApplyPanelArt()
{
	UiArt::PreloadCommon();

	UBorder* Target = PanelBorder ? PanelBorder.Get() : BackgroundBorder.Get();
	if (Target)
	{
		GameHudPrivate::StyleBorderBlankPanel2(Target, PanelMaxSize);
	}
	else
	{
		int32 Styled = 0;
		if (HUDOverlay)
		{
			GameHudPrivate::StyleBordersUnder(HUDOverlay, PanelMaxSize, Styled, 1);
		}
		if (Styled == 0)
		{
			GameHudPrivate::StyleBordersUnder(GetRootWidget(), PanelMaxSize, Styled, 1);
		}
	}

	ApplyHudTextColors();
}

void UGameHudWidget::ApplyHudTextColors()
{
	if (!GoldText && WidgetTree)
	{
		GoldText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("GoldText")));
	}
	if (!TowerHealthText && WidgetTree)
	{
		TowerHealthText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TowerHealthText")));
	}

	GameHudPrivate::ForceWhiteText(GoldText, HudTextColor);
	GameHudPrivate::ForceWhiteText(TowerHealthText, HudTextColor);

	// Also catch similarly named labels BP authors sometimes use.
	if (WidgetTree)
	{
		static const TCHAR* ExtraNames[] = {
			TEXT("GoldLabel"), TEXT("TxtGold"), TEXT("HealthText"), TEXT("TxtHealth"), TEXT("TowerHealth")
		};
		for (const TCHAR* Name : ExtraNames)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(WidgetTree->FindWidget(Name)))
			{
				GameHudPrivate::ForceWhiteText(Text, HudTextColor);
			}
		}
	}
}

void UGameHudWidget::ScheduleRestyle()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UGameHudWidget::ApplyPanelArt));
	}
}

void UGameHudWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyPanelArt();
}

void UGameHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyPanelArt();
	ScheduleRestyle();
	TextColorRefreshAt = 0.f;
}

void UGameHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// BP gold/health updates can reset text colour to black — keep HUD readable.
	TextColorRefreshAt -= InDeltaTime;
	if (TextColorRefreshAt <= 0.f)
	{
		ApplyHudTextColors();
		TextColorRefreshAt = 0.25f;
	}
}
