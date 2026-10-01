#include "PauseResumeSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogPauseResume, Log, All);

namespace PauseResumePrivate
{
	static bool GetBool(UObject* Obj, FName Name, bool DefaultValue)
	{
		if (!Obj)
		{
			return DefaultValue;
		}
		if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
		{
			return Prop->GetPropertyValue_InContainer(Obj);
		}
		return DefaultValue;
	}

	static void SetBool(UObject* Obj, FName Name, bool Value)
	{
		if (!Obj)
		{
			return;
		}
		if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
		{
			Prop->SetPropertyValue_InContainer(Obj, Value);
		}
	}

	static void CallNoArg(UObject* Obj, FName FunctionName)
	{
		if (!Obj)
		{
			return;
		}
		if (UFunction* Fn = Obj->FindFunction(FunctionName))
		{
			if (Fn->ParmsSize == 0)
			{
				Obj->ProcessEvent(Fn, nullptr);
			}
		}
	}
}

void UPauseResumeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bClassesResolved = false;
	bPauseMenuWasVisible = false;
	ClassResolveRetryAt = 0.f;
}

void UPauseResumeSubsystem::Deinitialize()
{
	GameManagerClass = nullptr;
	PauseMenuClass = nullptr;
	Super::Deinitialize();
}

TStatId UPauseResumeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPauseResumeSubsystem, STATGROUP_Tickables);
}

bool UPauseResumeSubsystem::IsGameplayWorld() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const EWorldType::Type Type = World->WorldType;
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

void UPauseResumeSubsystem::ResolveClasses()
{
	if (bClassesResolved)
	{
		return;
	}

	GameManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));
	PauseMenuClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_PauseMenu.WBP_PauseMenu_C"));
	bClassesResolved = GameManagerClass != nullptr && PauseMenuClass != nullptr;
	if (bClassesResolved)
	{
		UE_LOG(LogPauseResume, Warning, TEXT("PauseResumeSubsystem active."));
	}
}

AActor* UPauseResumeSubsystem::FindGameManager() const
{
	if (!GameManagerClass)
	{
		return nullptr;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(World, GameManagerClass, Managers);
	return Managers.Num() > 0 ? Managers[0] : nullptr;
}

bool UPauseResumeSubsystem::IsPauseMenuVisible() const
{
	UWorld* World = GetWorld();
	if (!World || !PauseMenuClass)
	{
		return false;
	}

	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, PauseMenuClass, false);
	for (UUserWidget* Widget : Widgets)
	{
		if (!IsValid(Widget) || !Widget->IsInViewport())
		{
			continue;
		}
		const ESlateVisibility Vis = Widget->GetVisibility();
		if (Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden)
		{
			return true;
		}
	}
	return false;
}

uint8 UPauseResumeSubsystem::GetGameStateByte(AActor* GameManager) const
{
	if (!GameManager)
	{
		return 0;
	}

	
	if (FByteProperty* ByteProp = FindFProperty<FByteProperty>(GameManager->GetClass(), FName(TEXT("CurrentGameState"))))
	{
		return ByteProp->GetPropertyValue_InContainer(GameManager);
	}
	if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(GameManager->GetClass(), FName(TEXT("CurrentGameState"))))
	{
		if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
		{
			return static_cast<uint8>(Underlying->GetUnsignedIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(GameManager)));
		}
	}
	return 0;
}

void UPauseResumeSubsystem::ForceUnpause(const TCHAR* Reason)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(World, false);

	if (AActor* GM = FindGameManager())
	{
		PauseResumePrivate::SetBool(GM, FName(TEXT("bPaused")), false);

		if (UFunction* SetStateFn = GM->FindFunction(FName(TEXT("SetGameState"))))
		{
			// Playing = 0
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
					AsByte->SetPropertyValue_InContainer(Parms.GetData(), 0);
				}
				else if (FEnumProperty* AsEnum = CastField<FEnumProperty>(*It))
				{
					if (FNumericProperty* Underlying = AsEnum->GetUnderlyingProperty())
					{
						Underlying->SetIntPropertyValue(AsEnum->ContainerPtrToValuePtr<void>(Parms.GetData()), static_cast<int64>(0));
					}
				}
			}
			GM->ProcessEvent(SetStateFn, Parms.GetData());
		}

		PauseResumePrivate::CallNoArg(GM, FName(TEXT("ResumeGame")));
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		PC->SetPause(false);
		PC->bShowMouseCursor = true; // TD uses mouse for placement

		if (UFunction* InputFn = PC->FindFunction(FName(TEXT("SetInputMode_GameAndUIEx"))))
		{
			// Fall through — many BP wrappers need widgets; GameAndUI via native API below is enough.
		}

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}

	// Remove any leftover pause menus.
	if (PauseMenuClass)
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, PauseMenuClass, false);
		for (UUserWidget* Widget : Widgets)
		{
			if (IsValid(Widget))
			{
				Widget->RemoveFromParent();
			}
		}
	}

	UE_LOG(LogPauseResume, Warning, TEXT("ForceUnpause (%s)"), Reason);
}

void UPauseResumeSubsystem::Tick(float DeltaTime)
{
	if (!IsGameplayWorld())
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
		if (Now >= ClassResolveRetryAt)
		{
			ResolveClasses();
			ClassResolveRetryAt = Now + 1.f;
		}
		if (!bClassesResolved)
		{
			return;
		}
	}

	const bool bMenuVisible = IsPauseMenuVisible();
	const bool bWorldPaused = UGameplayStatics::IsGamePaused(World);
	AActor* GM = FindGameManager();
	const uint8 State = GetGameStateByte(GM);
	constexpr uint8 Playing = 0;
	constexpr uint8 Paused = 1;
	constexpr uint8 GameOver = 2;

	// Resume clicked: menu went away but world stayed paused.
	if (bPauseMenuWasVisible && !bMenuVisible && bWorldPaused && State != GameOver)
	{
		ForceUnpause(TEXT("pause menu closed"));
	}
	// State restored to Playing while world still paused.
	else if (bWorldPaused && State == Playing && !bMenuVisible)
	{
		ForceUnpause(TEXT("state is Playing"));
	}
	// bPaused cleared but world still paused.
	else if (bWorldPaused && GM && !PauseResumePrivate::GetBool(GM, FName(TEXT("bPaused")), true) && State != GameOver && !bMenuVisible)
	{
		ForceUnpause(TEXT("bPaused false"));
	}

	bPauseMenuWasVisible = bMenuVisible;

	// Keep GameManager flag aligned while menu is up.
	if (bMenuVisible && GM)
	{
		PauseResumePrivate::SetBool(GM, FName(TEXT("bPaused")), true);
	}
}
