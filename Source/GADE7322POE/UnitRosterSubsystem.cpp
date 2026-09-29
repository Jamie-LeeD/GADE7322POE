#include "UnitRosterSubsystem.h"

#include "BerserkerRageComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FrostAttackComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UnitReflectionUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnitRoster, Log, All);

namespace UnitRosterPrivate
{
	/** Chance that the next spawner enemy is a Berserker once they are unlocked. */
	static constexpr float BerserkerSpawnChance = 0.3f;

	/** Seconds of play before Berserkers can appear, so the opening wave stays goblin-only. */
	static constexpr float BerserkerUnlockTime = 20.f;

	/** Must match the cost literal in BP_DefenderPlacementManager::TryPlaceArcher (HasEnoughGold / SpendGold). */
	static constexpr int32 BaseArcherPlacementCost = 50;

	static constexpr int32 FallbackFrostArcherCost = 90;

	static constexpr int32 MsgKeyMode = 73220;
	static constexpr int32 MsgKeyPlacement = 73221;

	template <typename TComponent>
	static void EnsureComponent(AActor* Actor)
	{
		if (IsValid(Actor) && !Actor->IsActorBeingDestroyed() && !Actor->FindComponentByClass<TComponent>())
		{
			TComponent* Comp = NewObject<TComponent>(Actor, MakeUniqueObjectName(Actor, TComponent::StaticClass()));
			Actor->AddInstanceComponent(Comp);
			Comp->RegisterComponent();
		}
	}

	static FClassProperty* FindClassProp(const AActor* Actor, FName Name)
	{
		return Actor ? FindFProperty<FClassProperty>(Actor->GetClass(), Name) : nullptr;
	}
}

void UUnitRosterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Random.GenerateNewSeed();
}

void UUnitRosterSubsystem::Deinitialize()
{
	KnownEnemies.Reset();
	KnownFrostArchers.Reset();
	Super::Deinitialize();
}

TStatId UUnitRosterSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUnitRosterSubsystem, STATGROUP_Tickables);
}

bool UUnitRosterSubsystem::IsGameplayWorld() const
{
	const UWorld* World = GetWorld();
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UUnitRosterSubsystem::ResolveClasses()
{
	GoblinClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinEnemy.BP_GoblinEnemy_C"));
	BerserkerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinBerserker.BP_GoblinBerserker_C"));
	FrostArcherClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_FrostArcher.BP_FrostArcher_C"));
	SpawnerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_EnemySpawner.BP_EnemySpawner_C"));
	PlacementManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_DefenderPlacementManager.BP_DefenderPlacementManager_C"));
	GameManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));

	bClassesResolved = GoblinClass && SpawnerClass && PlacementManagerClass && GameManagerClass;
	if (bClassesResolved)
	{
		UE_LOG(LogUnitRoster, Warning, TEXT("UnitRoster active (Berserker: %s, Frost Archer: %s)"),
			BerserkerClass ? TEXT("found") : TEXT("MISSING"), FrostArcherClass ? TEXT("found") : TEXT("MISSING"));
	}
}

AActor* UUnitRosterSubsystem::FindFirstActor(UClass* Class) const
{
	UWorld* World = GetWorld();
	if (!World || !Class)
	{
		return nullptr;
	}
	for (TActorIterator<AActor> It(World, Class); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

bool UUnitRosterSubsystem::IsGamePlaying(AActor* GameManager) const
{
	if (!IsValid(GameManager))
	{
		return false;
	}
	UFunction* IsPlayingFn = GameManager->FindFunction(FName(TEXT("IsPlaying")));
	if (!IsPlayingFn)
	{
		return true;
	}

	TArray<uint8> Parms;
	Parms.SetNumZeroed(FMath::Max<int32>(1, IsPlayingFn->ParmsSize));
	GameManager->ProcessEvent(IsPlayingFn, Parms.GetData());
	for (TFieldIterator<FProperty> It(IsPlayingFn); It && (It->PropertyFlags & CPF_Parm); ++It)
	{
		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(*It))
		{
			if (It->PropertyFlags & (CPF_ReturnParm | CPF_OutParm))
			{
				return BoolProp->GetPropertyValue_InContainer(Parms.GetData());
			}
		}
	}
	return true;
}

void UUnitRosterSubsystem::ShowMessage(int32 Key, const FString& Text, const FColor& Color, float Seconds) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(Key), Seconds, Color, Text);
	}
	UE_LOG(LogUnitRoster, Log, TEXT("%s"), *Text);
}

int32 UUnitRosterSubsystem::GetFrostArcherCost() const
{
	if (FrostArcherClass)
	{
		const int32 Cost = FMath::RoundToInt(UnitReflection::GetNumeric(FrostArcherClass->GetDefaultObject(), FName(TEXT("PlacementCost")), 0.f));
		if (Cost > 0)
		{
			return Cost;
		}
	}
	return UnitRosterPrivate::FallbackFrostArcherCost;
}

bool UUnitRosterSubsystem::CallGoldFunction(AActor* GameManager, FName FunctionName, int32 Amount) const
{
	UFunction* Fn = IsValid(GameManager) ? GameManager->FindFunction(FunctionName) : nullptr;
	if (!Fn)
	{
		return false;
	}

	TArray<uint8> Parms;
	Parms.SetNumZeroed(FMath::Max<int32>(1, Fn->ParmsSize));
	bool bAmountSet = false;
	for (TFieldIterator<FProperty> It(Fn); It && (It->PropertyFlags & CPF_Parm); ++It)
	{
		FProperty* Prop = *It;
		if (bAmountSet || (Prop->PropertyFlags & (CPF_ReturnParm | CPF_OutParm)))
		{
			continue;
		}
		if (FIntProperty* AsInt = CastField<FIntProperty>(Prop))
		{
			AsInt->SetPropertyValue_InContainer(Parms.GetData(), Amount);
			bAmountSet = true;
		}
		else if (FDoubleProperty* AsDouble = CastField<FDoubleProperty>(Prop))
		{
			AsDouble->SetPropertyValue_InContainer(Parms.GetData(), static_cast<double>(Amount));
			bAmountSet = true;
		}
		else if (FFloatProperty* AsFloat = CastField<FFloatProperty>(Prop))
		{
			AsFloat->SetPropertyValue_InContainer(Parms.GetData(), static_cast<float>(Amount));
			bAmountSet = true;
		}
	}
	if (!bAmountSet)
	{
		return false;
	}

	GameManager->ProcessEvent(Fn, Parms.GetData());

	for (TFieldIterator<FProperty> It(Fn); It && (It->PropertyFlags & CPF_Parm); ++It)
	{
		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(*It))
		{
			if (It->PropertyFlags & (CPF_ReturnParm | CPF_OutParm))
			{
				return BoolProp->GetPropertyValue_InContainer(Parms.GetData());
			}
		}
	}
	return true;
}

void UUnitRosterSubsystem::SetFrostPlacementMode(bool bFrost)
{
	AActor* PlacementManager = FindFirstActor(PlacementManagerClass);
	FClassProperty* ArcherClassProp = UnitRosterPrivate::FindClassProp(PlacementManager, FName(TEXT("ArcherClass")));
	if (!ArcherClassProp)
	{
		return;
	}

	UClass* Current = Cast<UClass>(ArcherClassProp->GetObjectPropertyValue_InContainer(PlacementManager));
	if (!OriginalArcherClass && Current && Current != FrostArcherClass)
	{
		OriginalArcherClass = Current;
	}

	if (bFrost && !FrostArcherClass)
	{
		ShowMessage(UnitRosterPrivate::MsgKeyMode, TEXT("BP_FrostArcher asset not found in /Game/Defenders"), FColor::Red);
		return;
	}

	ArcherClassProp->SetObjectPropertyValue_InContainer(PlacementManager, bFrost ? FrostArcherClass.Get() : OriginalArcherClass.Get());
	bFrostMode = bFrost;

	if (bFrost)
	{
		ShowMessage(UnitRosterPrivate::MsgKeyMode, FString::Printf(TEXT("Placing: FROST ARCHER (%d gold) - slows enemies"), GetFrostArcherCost()), FColor::Cyan);
	}
	else
	{
		ShowMessage(UnitRosterPrivate::MsgKeyMode, FString::Printf(TEXT("Placing: ROYAL ARCHER (%d gold)"), UnitRosterPrivate::BaseArcherPlacementCost), FColor::Yellow);
	}
}

void UUnitRosterSubsystem::TickPlacementInput()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	const bool bOne = PC->IsInputKeyDown(EKeys::One) || PC->IsInputKeyDown(EKeys::NumPadOne);
	const bool bTwo = PC->IsInputKeyDown(EKeys::Two) || PC->IsInputKeyDown(EKeys::NumPadTwo);
	if (bOne && !bKeyOneDown)
	{
		SetFrostPlacementMode(false);
	}
	if (bTwo && !bKeyTwoDown)
	{
		SetFrostPlacementMode(true);
	}
	bKeyOneDown = bOne;
	bKeyTwoDown = bTwo;
}

void UUnitRosterSubsystem::TickFrostPlacementCharges()
{
	UWorld* World = GetWorld();
	if (!World || !FrostArcherClass)
	{
		return;
	}

	for (auto It = KnownFrostArchers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	for (TActorIterator<AActor> It(World, FrostArcherClass); It; ++It)
	{
		AActor* Archer = *It;
		if (!IsValid(Archer) || Archer->IsActorBeingDestroyed() || KnownFrostArchers.Contains(Archer))
		{
			continue;
		}
		KnownFrostArchers.Add(Archer);
		if (!bFrostArchersSeeded)
		{
			continue;
		}

		// TryPlaceArcher already checked the slot and charged BaseArcherPlacementCost.
		const int32 FrostCost = GetFrostArcherCost();
		const int32 Surcharge = FrostCost - UnitRosterPrivate::BaseArcherPlacementCost;
		if (Surcharge <= 0)
		{
			continue;
		}

		AActor* GameManager = FindFirstActor(GameManagerClass);
		const int32 Gold = FMath::RoundToInt(UnitReflection::GetNumeric(GameManager, FName(TEXT("Gold")), 0.f));
		if (Gold >= Surcharge && CallGoldFunction(GameManager, FName(TEXT("SpendGold")), Surcharge))
		{
			ShowMessage(UnitRosterPrivate::MsgKeyPlacement, FString::Printf(TEXT("Frost Archer placed (-%d gold)"), FrostCost), FColor::Cyan);
			continue;
		}

		// Can't afford the Frost Archer: refund the base cost and free the slot again.
		CallGoldFunction(GameManager, FName(TEXT("AddGold")), UnitRosterPrivate::BaseArcherPlacementCost);
		if (AActor* PlacementManager = FindFirstActor(PlacementManagerClass))
		{
			if (UFunction* NotifyFn = PlacementManager->FindFunction(FName(TEXT("NotifyDefenderDestroyed"))))
			{
				TArray<uint8> Parms;
				Parms.SetNumZeroed(FMath::Max<int32>(1, NotifyFn->ParmsSize));
				for (TFieldIterator<FProperty> ParmIt(NotifyFn); ParmIt && (ParmIt->PropertyFlags & CPF_Parm); ++ParmIt)
				{
					if (FObjectProperty* ObjProp = CastField<FObjectProperty>(*ParmIt))
					{
						if (Archer->IsA(ObjProp->PropertyClass))
						{
							ObjProp->SetObjectPropertyValue_InContainer(Parms.GetData(), Archer);
						}
						break;
					}
				}
				PlacementManager->ProcessEvent(NotifyFn, Parms.GetData());
			}
		}
		Archer->Destroy();
		ShowMessage(UnitRosterPrivate::MsgKeyPlacement, FString::Printf(TEXT("Not enough gold for a Frost Archer (%d)"), FrostCost), FColor::Red);
	}
	bFrostArchersSeeded = true;
}

void UUnitRosterSubsystem::TickSpawnMix(float Now)
{
	UWorld* World = GetWorld();
	if (!World || !BerserkerClass)
	{
		return;
	}

	AActor* Spawner = FindFirstActor(SpawnerClass);
	FClassProperty* GoblinClassProp = UnitRosterPrivate::FindClassProp(Spawner, FName(TEXT("GoblinClass")));
	if (!GoblinClassProp)
	{
		return;
	}

	UClass* Current = Cast<UClass>(GoblinClassProp->GetObjectPropertyValue_InContainer(Spawner));
	if (Current != GoblinClass && Current != BerserkerClass)
	{
		return;
	}

	for (auto It = KnownEnemies.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
	for (TActorIterator<AActor> It(World, GoblinClass); It; ++It)
	{
		if (IsValid(*It) && !KnownEnemies.Contains(*It))
		{
			KnownEnemies.Add(*It);
			bSpawnRollPending = true;
		}
	}

	if (!bSpawnRollPending)
	{
		return;
	}
	bSpawnRollPending = false;

	const bool bUnlocked = PlayStartTime >= 0.f && (Now - PlayStartTime) >= UnitRosterPrivate::BerserkerUnlockTime;
	const bool bBerserker = bUnlocked && Random.FRand() < UnitRosterPrivate::BerserkerSpawnChance;
	GoblinClassProp->SetObjectPropertyValue_InContainer(Spawner, bBerserker ? BerserkerClass.Get() : GoblinClass.Get());
}

void UUnitRosterSubsystem::EnsureVariantComponents()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (BerserkerClass)
	{
		for (TActorIterator<AActor> It(World, BerserkerClass); It; ++It)
		{
			UnitRosterPrivate::EnsureComponent<UBerserkerRageComponent>(*It);
		}
	}
	if (FrostArcherClass)
	{
		for (TActorIterator<AActor> It(World, FrostArcherClass); It; ++It)
		{
			UnitRosterPrivate::EnsureComponent<UFrostAttackComponent>(*It);
		}
	}
}

void UUnitRosterSubsystem::Tick(float DeltaTime)
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
		if (Now < ClassResolveRetryAt)
		{
			return;
		}
		ResolveClasses();
		ClassResolveRetryAt = Now + 1.f;
		if (!bClassesResolved)
		{
			return;
		}
	}

	if (Now >= NextComponentCheckTime)
	{
		EnsureVariantComponents();
		NextComponentCheckTime = Now + 0.1f;
	}

	if (!IsGamePlaying(FindFirstActor(GameManagerClass)))
	{
		return;
	}

	if (PlayStartTime < 0.f)
	{
		PlayStartTime = Now;
		if (FrostArcherClass)
		{
			ShowMessage(UnitRosterPrivate::MsgKeyMode, FString::Printf(TEXT("Press 1: Royal Archer (%d gold)   |   Press 2: Frost Archer (%d gold)"),
				UnitRosterPrivate::BaseArcherPlacementCost, GetFrostArcherCost()), FColor::White, 8.f);
		}
	}

	TickPlacementInput();
	TickFrostPlacementCharges();
	TickSpawnMix(Now);
}
