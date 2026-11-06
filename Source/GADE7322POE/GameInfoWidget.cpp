#include "GameInfoWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UiArtLibrary.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameInfo, Log, All);

namespace GameInfoPrivate
{
	static UTextBlock* MakeLabel(UWidgetTree* Tree, const FString& Name, const FText& Text, int32 Size, FLinearColor Color,
		ETextJustify::Type Justify = ETextJustify::Left)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
		Label->SetText(Text);
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetJustification(Justify);
		Label->SetAutoWrapText(true);
		return Label;
	}

	static void SetGameManagerPaused(UWorld* World, bool bPaused)
	{
		if (!World)
		{
			return;
		}
		UClass* GMClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));
		if (!GMClass)
		{
			return;
		}
		TArray<AActor*> Managers;
		UGameplayStatics::GetAllActorsOfClass(World, GMClass, Managers);
		AActor* GM = Managers.Num() > 0 ? Managers[0] : nullptr;
		if (!GM)
		{
			return;
		}

		if (UFunction* SetStateFn = GM->FindFunction(FName(TEXT("SetGameState"))))
		{
			// E_GameState: Playing=0, Paused=1
			const uint8 State = bPaused ? 1 : 0;
			TArray<uint8> Parms;
			Parms.SetNumZeroed(SetStateFn->ParmsSize);
			for (TFieldIterator<FProperty> It(SetStateFn); It && (It->PropertyFlags & CPF_Parm); ++It)
			{
				if (It->PropertyFlags & CPF_ReturnParm)
				{
					continue;
				}
				if (FByteProperty* AsByte = CastField<FByteProperty>(*It))
				{
					AsByte->SetPropertyValue_InContainer(Parms.GetData(), State);
				}
				else if (FEnumProperty* AsEnum = CastField<FEnumProperty>(*It))
				{
					if (FNumericProperty* Underlying = AsEnum->GetUnderlyingProperty())
					{
						Underlying->SetIntPropertyValue(AsEnum->ContainerPtrToValuePtr<void>(Parms.GetData()), static_cast<int64>(State));
					}
				}
			}
			GM->ProcessEvent(SetStateFn, Parms.GetData());
		}

		if (FBoolProperty* PausedProp = FindFProperty<FBoolProperty>(GM->GetClass(), FName(TEXT("bPaused"))))
		{
			PausedProp->SetPropertyValue_InContainer(GM, bPaused);
		}
		if (!bPaused)
		{
			if (UFunction* ResumeFn = GM->FindFunction(FName(TEXT("ResumeGame"))))
			{
				if (ResumeFn->ParmsSize == 0)
				{
					GM->ProcessEvent(ResumeFn, nullptr);
				}
			}
		}
	}
}

UGameInfoWidget* UGameInfoWidget::OpenInfoPanel(UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return nullptr;
	}

	TArray<UUserWidget*> Existing;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Existing, UGameInfoWidget::StaticClass(), false);
	for (UUserWidget* W : Existing)
	{
		if (UGameInfoWidget* Info = Cast<UGameInfoWidget>(W))
		{
			if (Info->IsInViewport())
			{
				return Info;
			}
		}
	}

	UClass* WidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/WBP_GameInfo.WBP_GameInfo_C"));
	if (!WidgetClass)
	{
		WidgetClass = UGameInfoWidget::StaticClass();
	}
	UGameInfoWidget* Info = CreateWidget<UGameInfoWidget>(PC, WidgetClass);
	if (!Info)
	{
		return nullptr;
	}
	Info->AddToViewport(80);
	Info->ApplyPausedState(true);
	UE_LOG(LogGameInfo, Warning, TEXT("Opened game info panel (paused)"));
	return Info;
}

bool UGameInfoWidget::IsInfoPanelOpen(UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return false;
	}
	TArray<UUserWidget*> Existing;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Existing, UGameInfoWidget::StaticClass(), false);
	for (UUserWidget* W : Existing)
	{
		if (IsValid(W) && W->IsInViewport())
		{
			const ESlateVisibility Vis = W->GetVisibility();
			if (Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden)
			{
				return true;
			}
		}
	}
	return false;
}

void UGameInfoWidget::BindEvents()
{
	if (bEventsBound || !CloseButton)
	{
		return;
	}
	CloseButton->OnClicked.AddUniqueDynamic(this, &UGameInfoWidget::HandleCloseClicked);
	bEventsBound = true;
}

void UGameInfoWidget::ApplyArtStyles()
{
	if (PanelBorder)
	{
		const FVector2D PanelSize = UiArt::GetFittedSize(TEXT("blankpanel1"), PanelMaxSize);
		UiArt::StyleBorderImage(PanelBorder, TEXT("blankpanel1"), PanelMaxSize);
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(PanelBorder->Slot))
		{
			PanelSlot->SetSize(PanelSize.X >= 32.f && PanelSize.Y >= 32.f ? PanelSize : PanelMaxSize);
		}
	}
	if (CloseButton)
	{
		const FVector2D CloseSize = UiArt::GetFittedSize(TEXT("xbttn"), CloseButtonMaxSize);
		UiArt::StyleImageButton(CloseButton, TEXT("xbttn"), CloseButtonMaxSize);
		if (UCanvasPanelSlot* CloseSlot = Cast<UCanvasPanelSlot>(CloseButton->Slot))
		{
			CloseSlot->SetSize(CloseSize.X >= 32.f && CloseSize.Y >= 32.f ? CloseSize : CloseButtonMaxSize);
		}
	}
}

void UGameInfoWidget::ScheduleRestyle()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UGameInfoWidget::ApplyArtStyles));
	}
}

void UGameInfoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UiArt::PreloadCommon();
	BuildLayout();
	BindEvents();
	ApplyArtStyles();
}

void UGameInfoWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildLayout();
	ApplyArtStyles();
}

void UGameInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UiArt::PreloadCommon();
	BuildLayout();
	BindEvents();
	ApplyArtStyles();
	ScheduleRestyle();
	SetVisibility(ESlateVisibility::Visible);
}

void UGameInfoWidget::NativeDestruct()
{
	if (bDidPause)
	{
		ApplyPausedState(false);
		bDidPause = false;
	}
	Super::NativeDestruct();
}

void UGameInfoWidget::ApplyPausedState(bool bPaused)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(World, bPaused);
	GameInfoPrivate::SetGameManagerPaused(World, bPaused);
	bDidPause = bPaused;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		PC->bShowMouseCursor = true;
		if (bPaused)
		{
			FInputModeUIOnly Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetWidgetToFocus(TakeWidget());
			PC->SetInputMode(Mode);
		}
		else
		{
			FInputModeGameAndUI Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
		}
	}
}

void UGameInfoWidget::AddSection(UScrollBox* Scroll, const FString& Title, const FString& Body)
{
	if (!Scroll || !WidgetTree)
	{
		return;
	}
	UTextBlock* TitleLabel = GameInfoPrivate::MakeLabel(WidgetTree, Title + TEXT("_T"), FText::FromString(Title), 22,
		FLinearColor(1.f, 0.78f, 0.45f));
	UTextBlock* BodyLabel = GameInfoPrivate::MakeLabel(WidgetTree, Title + TEXT("_B"), FText::FromString(Body), 16,
		FLinearColor(0.95f, 0.93f, 0.88f));
	Scroll->AddChild(TitleLabel);
	Scroll->AddChild(BodyLabel);
}

void UGameInfoWidget::BuildLayout()
{
	// Designer WBP already has a close button — keep that tree.
	if (CloseButton && PanelBorder)
	{
		bLayoutBuilt = true;
		return;
	}
	if (bLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bLayoutBuilt = true;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.06f, 0.78f));
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	FVector2D PanelSize = UiArt::GetFittedSize(TEXT("blankpanel1"), PanelMaxSize);
	if (PanelSize.X < 32.f || PanelSize.Y < 32.f)
	{
		PanelSize = PanelMaxSize;
	}
	PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	UiArt::StyleBorderImage(PanelBorder, TEXT("blankpanel1"), PanelMaxSize);
	PanelBorder->SetPadding(FMargin(48.f, 56.f, 48.f, 48.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetSize(PanelSize);
	}

	UCanvasPanel* PanelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PanelCanvas"));
	PanelBorder->SetContent(PanelCanvas);

	FVector2D CloseSize = UiArt::GetFittedSize(TEXT("xbttn"), CloseButtonMaxSize);
	if (CloseSize.X < 32.f || CloseSize.Y < 32.f)
	{
		CloseSize = CloseButtonMaxSize;
	}
	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	UiArt::StyleImageButton(CloseButton, TEXT("xbttn"), CloseButtonMaxSize);
	if (UCanvasPanelSlot* CloseSlot = PanelCanvas->AddChildToCanvas(CloseButton))
	{
		CloseSlot->SetAnchors(FAnchors(1.f, 0.f));
		CloseSlot->SetAlignment(FVector2D(1.f, 0.f));
		CloseSlot->SetPosition(FVector2D(-8.f, 8.f));
		CloseSlot->SetSize(CloseSize);
		CloseSlot->SetZOrder(2);
	}

	// Dark readable plate over the ornate panel art so text stays visible.
	UBorder* ContentPlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ContentPlate"));
	ContentPlate->SetBrushColor(FLinearColor(0.04f, 0.03f, 0.05f, 0.82f));
	ContentPlate->SetPadding(FMargin(18.f, 14.f));
	if (UCanvasPanelSlot* PlateSlot = PanelCanvas->AddChildToCanvas(ContentPlate))
	{
		PlateSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		PlateSlot->SetOffsets(FMargin(36.f, 48.f, 96.f, 40.f));
	}

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	ContentPlate->SetContent(Content);

	UTextBlock* Heading = GameInfoPrivate::MakeLabel(WidgetTree, TEXT("Heading"),
		FText::FromString(TEXT("The Grimm Tower — Field Guide")), 28, FLinearColor(1.f, 0.82f, 0.4f), ETextJustify::Center);
	if (UVerticalBoxSlot* HeadSlot = Content->AddChildToVerticalBox(Heading))
	{
		HeadSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		HeadSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	USizeBox* ScrollSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ScrollSize"));
	ScrollSize->SetHeightOverride(680.f);
	InfoScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("InfoScroll"));
	UScrollBox* Scroll = InfoScroll;
	ScrollSize->AddChild(Scroll);
	Content->AddChildToVerticalBox(ScrollSize);

	AddSection(Scroll, TEXT("Defenders"),
		TEXT("Royal Archer — Reliable ranged damage. Good all-rounder for lanes.\n\n"
			 "Frost Archer — Shots slow enemies by 40% for 2 seconds. Great for stalling crowds.\n\n"
			 "Royal Guardian — Tough melee tank. Shield Guard reduces damage taken by 50% for 3s when foes get close (12s cooldown)."));

	AddSection(Scroll, TEXT("Upgrades"),
		TEXT("Upgrade buttons sit under each defender on the bar.\n"
			 "Buying an upgrade powers up EVERY unit of that type (already placed and future ones).\n\n"
			 "Each level: +35% Max Health and +20% Attack Damage.\n"
			 "Cost rises each time (about ×1.75).\n"
			 "Max 2 levels per type.\n\n"
			 "Aura colours:\n"
			 "• Level 1 — blue glow\n"
			 "• Level 2 — pink glow"));

	AddSection(Scroll, TEXT("Enemies"),
		TEXT("Goblin — Basic melee grunt that chases your defenders and the princess tower.\n\n"
			 "Berserker (Minotaur) — When low on health (≤30%), enters Rage: faster and hits harder for a few seconds.\n\n"
			 "Shaman (Necromancer) — Fires magic bolts and periodically buffs nearby goblins (+speed and +damage).\n\n"
			 "Elite Goblins — Occasional tougher variants with more HP, different speed, bonus gold, and a distinct look."));

	AddSection(Scroll, TEXT("Tips"),
		TEXT("Place defenders on free pads. Use Frost to slow elites and berserkers.\n"
			 "Upgrade your main damage type early, then tank/support as waves grow.\n"
			 "Watch the red screen edge — that means the princess tower is in danger!"));
}

void UGameInfoWidget::HandleCloseClicked()
{
	CloseAndResume();
}

void UGameInfoWidget::CloseAndResume()
{
	ApplyPausedState(false);
	bDidPause = false;
	RemoveFromParent();
	UE_LOG(LogGameInfo, Warning, TEXT("Closed game info panel (resumed)"));
}
