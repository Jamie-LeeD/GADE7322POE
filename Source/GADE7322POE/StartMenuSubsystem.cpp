#include "StartMenuSubsystem.h"

#include "BudgetWaveStrategy.h"
#include "CounterWaveStrategy.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "StartMenuWidget.h"
#include "UiArtLibrary.h"
#include "UObject/UnrealType.h"
#include "WaveDirectorSettings.h"
#include "WaveDirectorSubsystem.h"
#include "WaveStrategy.h"

DEFINE_LOG_CATEGORY_STATIC(LogStartMenu, Log, All);

void UStartMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bClassesResolved = false;
	bMenuShown = false;
	bMatchStarted = false;
}

void UStartMenuSubsystem::Deinitialize()
{
	if (IsValid(MenuWidget))
	{
		MenuWidget->RemoveFromParent();
	}
	MenuWidget = nullptr;
	GameManagerClass = nullptr;
	Super::Deinitialize();
}

TStatId UStartMenuSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStartMenuSubsystem, STATGROUP_Tickables);
}

bool UStartMenuSubsystem::IsGameplayWorld() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const EWorldType::Type Type = World->WorldType;
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

void UStartMenuSubsystem::ResolveClasses()
{
	if (bClassesResolved)
	{
		return;
	}
	GameManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));
	bClassesResolved = GameManagerClass != nullptr;
}

AActor* UStartMenuSubsystem::FindGameManager() const
{
	if (!GameManagerClass)
	{
		return nullptr;
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), GameManagerClass, Found);
	return Found.Num() > 0 ? Found[0] : nullptr;
}

void UStartMenuSubsystem::SetGameStatePlaying(bool bPlaying)
{
	AActor* GM = FindGameManager();
	if (!IsValid(GM))
	{
		return;
	}

	if (UFunction* SetStateFn = GM->FindFunction(FName(TEXT("SetGameState"))))
	{
		// E_GameState: Playing=0, Paused=1, GameOver=2
		const uint8 State = bPlaying ? 0 : 1;
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
		PausedProp->SetPropertyValue_InContainer(GM, !bPlaying);
	}
}

void UStartMenuSubsystem::ApplyInputForMenu()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!PC)
	{
		return;
	}
	PC->bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (MenuWidget)
	{
		Mode.SetWidgetToFocus(MenuWidget->TakeWidget());
	}
	PC->SetInputMode(Mode);
}

void UStartMenuSubsystem::ApplyInputForGame()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!PC)
	{
		return;
	}
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

void UStartMenuSubsystem::ShowMenu()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!PC)
	{
		return;
	}

	UiArt::PreloadCommon();

	UClass* MenuClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/WBP_StartMenu.WBP_StartMenu_C"));
	if (!MenuClass)
	{
		MenuClass = UStartMenuWidget::StaticClass();
	}
	MenuWidget = CreateWidget<UStartMenuWidget>(PC, MenuClass);
	if (!MenuWidget)
	{
		UE_LOG(LogStartMenu, Error, TEXT("Failed to create StartMenuWidget"));
		return;
	}
	MenuWidget->OnStrategyChosen.AddDynamic(this, &UStartMenuSubsystem::HandleStrategyChosen);
	MenuWidget->AddToViewport(100);
	SetGameStatePlaying(false);
	ApplyInputForMenu();
	bMenuShown = true;
	UE_LOG(LogStartMenu, Warning, TEXT("Start menu shown — pick Play, then Budget or Counter"));
}

void UStartMenuSubsystem::HandleStrategyChosen(bool bBudgetStrategy)
{
	if (bMatchStarted)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UWaveDirectorSubsystem* Director = World->GetSubsystem<UWaveDirectorSubsystem>())
	{
		TSubclassOf<UWaveStrategy> Class = bBudgetStrategy
			? TSubclassOf<UWaveStrategy>(UBudgetWaveStrategy::StaticClass())
			: TSubclassOf<UWaveStrategy>(UCounterWaveStrategy::StaticClass());
		Director->SetStrategyClass(Class);
	}

	SetGameStatePlaying(true);
	ApplyInputForGame();

	if (IsValid(MenuWidget))
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}

	bMatchStarted = true;
	UE_LOG(LogStartMenu, Warning, TEXT("Match started with %s strategy"),
		bBudgetStrategy ? TEXT("Threat Budget") : TEXT("Counter-Play"));

	if (GEngine && GetDefault<UWaveDirectorSettings>()->bShowDebugHud)
	{
		GEngine->AddOnScreenDebugMessage(7322500, 4.f, FColor::Green,
			bBudgetStrategy ? TEXT("Starting: Threat Budget") : TEXT("Starting: Counter-Play"));
	}
}

void UStartMenuSubsystem::Tick(float DeltaTime)
{
	if (!IsGameplayWorld() || bMatchStarted)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (!bClassesResolved)
	{
		if (Now < ClassResolveRetryAt)
		{
			return;
		}
		ResolveClasses();
		if (!bClassesResolved)
		{
			ClassResolveRetryAt = Now + 0.5f;
			return;
		}
	}

	if (!bMenuShown)
	{
		// Wait one beat so GameManager / PlayerController exist.
		if (FindGameManager() && UGameplayStatics::GetPlayerController(World, 0))
		{
			ShowMenu();
		}
	}
	else
	{
		// Keep match paused until a strategy is chosen (GameManager may flip back to Playing on BeginPlay).
		SetGameStatePlaying(false);
	}
}
