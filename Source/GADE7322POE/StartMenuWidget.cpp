#include "StartMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Styling/SlateBrush.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UiArtLibrary.h"

namespace StartMenuPrivate
{
	static UTextBlock* MakeLabel(UWidgetTree* Tree, const FString& Name, const FText& Text, int32 Size, FLinearColor Color)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
		Label->SetText(Text);
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(true);
		return Label;
	}

	static UButton* MakeArtButton(UWidgetTree* Tree, const FString& Name, const TCHAR* TextureName, FVector2D MaxSize)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), *Name);
		UiArt::StyleImageButton(Button, TextureName, MaxSize);
		return Button;
	}

	static UButton* MakeLabeledArtButton(UWidgetTree* Tree, const FString& Name, const FText& Label, int32 FontSize,
		const TCHAR* TextureName, FVector2D MaxSize)
	{
		UButton* Button = MakeArtButton(Tree, Name, TextureName, MaxSize);
		UTextBlock* Text = MakeLabel(Tree, Name + TEXT("_Label"), Label, FontSize, FLinearColor(1.f, 0.85f, 0.95f));
		Button->AddChild(Text);
		return Button;
	}

	static USizeBox* WrapInSizeBox(UWidgetTree* Tree, const FString& Name, UWidget* Child, FVector2D MaxSize, const TCHAR* TextureName)
	{
		// Seed with MaxSize so a first-frame texture miss cannot lock a 1x1 box.
		const FVector2D Fitted = UiArt::GetFittedSize(TextureName, MaxSize);
		const FVector2D Seed = (Fitted.X >= 32.f && Fitted.Y >= 32.f) ? Fitted : MaxSize;
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *Name);
		Box->SetWidthOverride(Seed.X);
		Box->SetHeightOverride(Seed.Y);
		Box->AddChild(Child);
		return Box;
	}
}

void UStartMenuWidget::BindButtonEvents()
{
	if (bEventsBound)
	{
		return;
	}
	if (PlayButton)
	{
		PlayButton->OnClicked.AddUniqueDynamic(this, &UStartMenuWidget::HandlePlayClicked);
	}
	if (BudgetButton)
	{
		BudgetButton->OnClicked.AddUniqueDynamic(this, &UStartMenuWidget::HandleBudgetClicked);
	}
	if (CounterButton)
	{
		CounterButton->OnClicked.AddUniqueDynamic(this, &UStartMenuWidget::HandleCounterClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UStartMenuWidget::HandleBackClicked);
	}
	bEventsBound = true;
}

void UStartMenuWidget::ApplyArtStyles()
{
	// Always size from Class Defaults / UPROPERTY max boxes — never from SizeBox
	// (a bad first-frame fit used to stick forever via SizeFromBoxOrDefault).
	if (GreyBackdrop)
	{
		// Stretch blank panel across the whole screen (Box draw fills the border geometry).
		UiArt::StyleBorderImage(GreyBackdrop, TEXT("blankpanel1"));
		FSlateBrush GreyBrush = GreyBackdrop->Background;
		GreyBrush.DrawAs = ESlateBrushDrawType::Box;
		if (GreyBrush.GetResourceObject())
		{
			GreyBrush.Margin = FMargin(0.4f);
		}
		GreyBrush.TintColor = FSlateColor(FLinearColor::White);
		GreyBackdrop->SetBrush(GreyBrush);
		GreyBackdrop->SetBrushColor(FLinearColor(0.55f, 0.55f, 0.58f, 1.f));
	}
	if (BackgroundBorder)
	{
		UiArt::StyleBorderImage(BackgroundBorder, TEXT("startscreenpanel"));
	}
	if (TitleImage)
	{
		TitleImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PlayButton)
	{
		UiArt::StyleImageButton(PlayButton, TEXT("playbttn"), PlayButtonMaxSize);
	}
	if (BudgetButton)
	{
		UiArt::StyleImageButton(BudgetButton, TEXT("threatbudgetbttn"), StrategyButtonMaxSize);
	}
	if (CounterButton)
	{
		UiArt::StyleImageButton(CounterButton, TEXT("counterplaybttn"), StrategyButtonMaxSize);
	}
	if (BackButton)
	{
		UiArt::StyleImageButton(BackButton, TEXT("backbttn"), BackButtonMaxSize);
	}
	if (StrategyPanel)
	{
		UiArt::StyleBorderImage(StrategyPanel, TEXT("blankpanel2"), StrategyPanelMaxSize);
	}
}

void UStartMenuWidget::ScheduleRestyle()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UStartMenuWidget::ApplyArtStyles));
	}
}

void UStartMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UiArt::PreloadCommon();
	BuildLayout();
	BindButtonEvents();
	ApplyArtStyles();
}

void UStartMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildLayout();
	ApplyArtStyles();
}

void UStartMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UiArt::PreloadCommon();
	BuildLayout();
	BindButtonEvents();
	ApplyArtStyles();
	ScheduleRestyle();
	ShowTitlePage();
}

void UStartMenuWidget::BuildLayout()
{
	// WBP already authored buttons — do not replace the designer tree.
	if (PlayButton)
	{
		bLayoutBuilt = true;
		return;
	}
	if (bLayoutBuilt || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;

	// Full-screen grey blank behind the start-screen art (shows through transparent edges).
	GreyBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GreyBackdrop"));
	UiArt::StyleBorderImage(GreyBackdrop, TEXT("blankpanel1"));
	{
		FSlateBrush GreyBrush = GreyBackdrop->Background;
		GreyBrush.DrawAs = ESlateBrushDrawType::Box;
		if (GreyBrush.GetResourceObject())
		{
			GreyBrush.Margin = FMargin(0.4f);
		}
		GreyBrush.TintColor = FSlateColor(FLinearColor::White);
		GreyBackdrop->SetBrush(GreyBrush);
	}
	GreyBackdrop->SetBrushColor(FLinearColor(0.55f, 0.55f, 0.58f, 1.f));
	if (UCanvasPanelSlot* GreySlot = Root->AddChildToCanvas(GreyBackdrop))
	{
		GreySlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		GreySlot->SetOffsets(FMargin(0.f));
		GreySlot->SetZOrder(0);
	}

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
	UiArt::StyleBorderImage(BackgroundBorder, TEXT("startscreenpanel"));
	if (UCanvasPanelSlot* BgSlot = Root->AddChildToCanvas(BackgroundBorder))
	{
		BgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BgSlot->SetOffsets(FMargin(0.f));
		BgSlot->SetZOrder(1);
	}

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.35f));
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
		DimSlot->SetZOrder(2);
	}

	PageSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("PageSwitcher"));
	if (UCanvasPanelSlot* SwitchSlot = Root->AddChildToCanvas(PageSwitcher))
	{
		SwitchSlot->SetAnchors(FAnchors(0.5f, 0.55f));
		SwitchSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		SwitchSlot->SetAutoSize(true);
		SwitchSlot->SetZOrder(3);
	}

	UVerticalBox* TitlePage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TitlePage"));

	UTextBlock* Subtitle = StartMenuPrivate::MakeLabel(WidgetTree, TEXT("Subtitle"),
		FText::FromString(TEXT("Defend the princess")), 26, FLinearColor(1.f, 0.92f, 0.75f));
	PlayButton = StartMenuPrivate::MakeArtButton(WidgetTree, TEXT("PlayButton"), TEXT("playbttn"), PlayButtonMaxSize);
	USizeBox* PlaySize = StartMenuPrivate::WrapInSizeBox(WidgetTree, TEXT("PlaySize"), PlayButton, PlayButtonMaxSize, TEXT("playbttn"));

	if (UVerticalBoxSlot* S1 = TitlePage->AddChildToVerticalBox(Subtitle))
	{
		S1->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
		S1->SetHorizontalAlignment(HAlign_Center);
	}
	if (UVerticalBoxSlot* S2 = TitlePage->AddChildToVerticalBox(PlaySize))
	{
		S2->SetPadding(FMargin(8.f));
		S2->SetHorizontalAlignment(HAlign_Center);
	}
	PageSwitcher->AddChild(TitlePage);

	UVerticalBox* StratPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StrategyPage"));
	FVector2D StratPanelSize = UiArt::GetFittedSize(TEXT("blankpanel2"), StrategyPanelMaxSize);
	if (StratPanelSize.X < 32.f || StratPanelSize.Y < 32.f)
	{
		StratPanelSize = StrategyPanelMaxSize;
	}
	USizeBox* StratSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StrategyPanelSize"));
	StratSize->SetWidthOverride(StratPanelSize.X);
	StratSize->SetHeightOverride(StratPanelSize.Y);
	StrategyPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StrategyPanel"));
	UiArt::StyleBorderImage(StrategyPanel, TEXT("blankpanel2"), StratPanelSize);
	StrategyPanel->SetPadding(FMargin(28.f, 36.f));
	StratSize->AddChild(StrategyPanel);
	UVerticalBox* StratInner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StratInner"));
	StrategyPanel->SetContent(StratInner);

	UTextBlock* StratTitle = StartMenuPrivate::MakeLabel(WidgetTree, TEXT("StratTitle"),
		FText::FromString(TEXT("Choose wave strategy")), 32, FLinearColor(1.f, 0.88f, 0.55f));
	UTextBlock* StratHint = StartMenuPrivate::MakeLabel(WidgetTree, TEXT("StratHint"),
		FText::FromString(TEXT("Budget grows threat over time. Counter adapts to your defence.")), 18,
		FLinearColor(0.98f, 0.95f, 0.88f));
	// Art already includes labels — no extra TextBlock on top.
	BudgetButton = StartMenuPrivate::MakeArtButton(WidgetTree, TEXT("BudgetButton"), TEXT("threatbudgetbttn"), StrategyButtonMaxSize);
	CounterButton = StartMenuPrivate::MakeArtButton(WidgetTree, TEXT("CounterButton"), TEXT("counterplaybttn"), StrategyButtonMaxSize);
	BackButton = StartMenuPrivate::MakeArtButton(WidgetTree, TEXT("BackButton"), TEXT("backbttn"), BackButtonMaxSize);

	USizeBox* BudgetSize = StartMenuPrivate::WrapInSizeBox(WidgetTree, TEXT("BudgetSize"), BudgetButton, StrategyButtonMaxSize, TEXT("threatbudgetbttn"));
	USizeBox* CounterSize = StartMenuPrivate::WrapInSizeBox(WidgetTree, TEXT("CounterSize"), CounterButton, StrategyButtonMaxSize, TEXT("counterplaybttn"));
	USizeBox* BackSize = StartMenuPrivate::WrapInSizeBox(WidgetTree, TEXT("BackSize"), BackButton, BackButtonMaxSize, TEXT("backbttn"));

	auto AddCentered = [&](UWidget* Child, float PadBottom)
	{
		if (UVerticalBoxSlot* Slot = StratInner->AddChildToVerticalBox(Child))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, PadBottom));
			Slot->SetHorizontalAlignment(HAlign_Center);
		}
	};
	AddCentered(StratTitle, 8.f);
	AddCentered(StratHint, 20.f);
	AddCentered(BudgetSize, 12.f);
	AddCentered(CounterSize, 12.f);
	AddCentered(BackSize, 0.f);

	if (UVerticalBoxSlot* PanelSlot = StratPage->AddChildToVerticalBox(StratSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
	}
	PageSwitcher->AddChild(StratPage);

	bLayoutBuilt = true;
}

void UStartMenuWidget::ShowTitlePage()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(0);
	}
}

void UStartMenuWidget::ShowStrategyPage()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(1);
	}
}

void UStartMenuWidget::HandlePlayClicked()
{
	OnPlayClicked.Broadcast();
	ShowStrategyPage();
}

void UStartMenuWidget::HandleBudgetClicked()
{
	OnStrategyChosen.Broadcast(true);
}

void UStartMenuWidget::HandleCounterClicked()
{
	OnStrategyChosen.Broadcast(false);
}

void UStartMenuWidget::HandleBackClicked()
{
	ShowTitlePage();
}
