#include "OrnateMenuWidget.h"

#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "UiArtLibrary.h"

namespace OrnateMenuPrivate
{
	static void StylePausePanel(UBorder* Border, FVector2D MaxSize)
	{
		if (!Border)
		{
			return;
		}
		UiArt::StyleBorderImage(Border, TEXT("pausepanel"), MaxSize);
		FSlateBrush Brush = Border->Background;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		if (Brush.GetResourceObject())
		{
			Brush.Margin = FMargin(0.28f);
		}
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Border->SetBrush(Brush);
		Border->SetBrushColor(FLinearColor::White);
		Border->SetPadding(FMargin(36.f, 48.f, 36.f, 56.f));
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

void UOrnateMenuWidget::ApplyPanelArt()
{
	UiArt::PreloadCommon();

	UBorder* Target = PanelBorder ? PanelBorder.Get() : BackgroundBorder.Get();
	if (!Target && PanelOverlay)
	{
		Target = OrnateMenuPrivate::FindFirstBorder(PanelOverlay);
	}
	if (!Target)
	{
		Target = OrnateMenuPrivate::FindFirstBorder(GetRootWidget());
	}
	if (Target)
	{
		OrnateMenuPrivate::StylePausePanel(Target, PanelMaxSize);
	}
}

void UOrnateMenuWidget::ScheduleRestyle()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UOrnateMenuWidget::ApplyPanelArt));
	}
}

void UOrnateMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyPanelArt();
}

void UOrnateMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyPanelArt();
	ScheduleRestyle();
}
