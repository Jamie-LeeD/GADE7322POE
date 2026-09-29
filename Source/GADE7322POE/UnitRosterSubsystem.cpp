#include "UnitRosterSubsystem.h"

#include "BerserkerRageComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FrostAttackComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnitRoster, Log, All);

namespace UnitRosterPrivate
{
	/** Chance that the next spawner enemy is a Berserker once they are unlocked. */
	static constexpr float BerserkerSpawnChance = 0.3f;

	/** Seconds of play before Berserkers can appear, so the opening wave stays goblin-only. */
	static constexpr float BerserkerUnlockTime = 20.f;

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
	GameManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));

	bClassesResolved = GoblinClass && SpawnerClass && GameManagerClass;
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
	}
	TickSpawnMix(Now);
}
