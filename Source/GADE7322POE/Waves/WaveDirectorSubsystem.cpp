#include "Waves/WaveDirectorSubsystem.h"

#include "Waves/BudgetWaveStrategy.h"
#include "Waves/CounterWaveStrategy.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Units/UnitReflectionUtils.h"
#include "Units/UnitRosterSubsystem.h"
#include "UObject/UnrealType.h"
#include "Waves/WaveDirectorSettings.h"
#include "Waves/WaveStrategy.h"

DEFINE_LOG_CATEGORY_STATIC(LogWaveDirector, Log, All);

namespace WaveDirectorPrivate
{
	static constexpr int32 HudKeyBase = 7322100;

	static bool GetBool(const UObject* Obj, FName Name, bool DefaultValue)
	{
		if (!Obj)
		{
			return DefaultValue;
		}
		if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
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

	/** Calls a Blueprint function with no inputs that returns an int (e.g. GetPathCount). */
	static int32 CallIntGetter(UObject* Obj, FName FunctionName, int32 DefaultValue)
	{
		UFunction* Fn = Obj ? Obj->FindFunction(FunctionName) : nullptr;
		if (!Fn || Fn->ParmsSize == 0)
		{
			return DefaultValue;
		}

		TArray<uint8> Parms;
		Parms.SetNumZeroed(Fn->ParmsSize);
		Obj->ProcessEvent(Fn, Parms.GetData());
		for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
			{
				if (const FIntProperty* IntProp = CastField<FIntProperty>(*It))
				{
					return IntProp->GetPropertyValue_InContainer(Parms.GetData());
				}
			}
		}
		return DefaultValue;
	}

	static bool IsAlive(const AActor* Actor)
	{
		return IsValid(Actor) && !Actor->IsActorBeingDestroyed() && !GetBool(Actor, FName(TEXT("bIsDead")), false);
	}

	static UWaveDirectorSubsystem* GetDirector(UWorld* World)
	{
		return World ? World->GetSubsystem<UWaveDirectorSubsystem>() : nullptr;
	}

	static FAutoConsoleCommandWithWorldAndArgs StrategyCommand(
		TEXT("td.WaveStrategy"),
		TEXT("Switch the procedural wave strategy from the next wave: td.WaveStrategy A (Threat Budget) | B (Counter-Play)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UWaveDirectorSubsystem* Director = GetDirector(World);
			if (!Director || Args.Num() == 0)
			{
				return;
			}
			const FString Choice = Args[0].ToUpper();
			if (Choice == TEXT("A") || Choice == TEXT("BUDGET"))
			{
				Director->SetStrategyClass(UBudgetWaveStrategy::StaticClass());
			}
			else if (Choice == TEXT("B") || Choice == TEXT("COUNTER"))
			{
				Director->SetStrategyClass(UCounterWaveStrategy::StaticClass());
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs SkipCommand(
		TEXT("td.SkipIntermission"),
		TEXT("Start the next procedural wave immediately."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UWaveDirectorSubsystem* Director = GetDirector(World))
			{
				Director->SkipIntermission();
			}
		}));
}

void UWaveDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Phase = EPhase::Idle;
	CurrentWave = 0;
	bDirecting = false;
}

void UWaveDirectorSubsystem::Deinitialize()
{
	TrackedEnemies.Reset();
	TowerSamples.Reset();
	Lanes.Reset();
	Strategy = nullptr;
	Super::Deinitialize();
}

TStatId UWaveDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWaveDirectorSubsystem, STATGROUP_Tickables);
}

bool UWaveDirectorSubsystem::IsGameplayWorld() const
{
	const UWorld* World = GetWorld();
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UWaveDirectorSubsystem::ResolveClasses()
{
	GoblinClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinEnemy.BP_GoblinEnemy_C"));
	BerserkerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinBerserker.BP_GoblinBerserker_C"));
	ShamanClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinShaman.BP_GoblinShaman_C"));
	ArcherClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalArcher.BP_RoyalArcher_C"));
	FrostArcherClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_FrostArcher.BP_FrostArcher_C"));
	GuardianClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalGuardian.BP_RoyalGuardian_C"));
	SpawnerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_EnemySpawner.BP_EnemySpawner_C"));
	SpawnPointClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_EnemySpawnPoint.BP_EnemySpawnPoint_C"));
	TowerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Tower/BP_PrincessTower.BP_PrincessTower_C"));
	GameManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));

	bClassesResolved = GoblinClass && SpawnerClass && TowerClass && GameManagerClass;
	if (bClassesResolved)
	{
		UE_LOG(LogWaveDirector, Warning, TEXT("WaveDirector ready (Berserker: %s, Shaman: %s, spawn points: %s)"),
			BerserkerClass ? TEXT("found") : TEXT("MISSING"), ShamanClass ? TEXT("found") : TEXT("MISSING"),
			SpawnPointClass ? TEXT("found") : TEXT("MISSING"));
	}
}

AActor* UWaveDirectorSubsystem::FindFirstActor(UClass* Class) const
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

AActor* UWaveDirectorSubsystem::GetTower() const
{
	if (!CachedTower.IsValid())
	{
		CachedTower = FindFirstActor(TowerClass);
	}
	return CachedTower.Get();
}

bool UWaveDirectorSubsystem::IsGamePlaying(AActor* GameManager) const
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

void UWaveDirectorSubsystem::EnsureStrategy(bool bApplyPending)
{
	if (Strategy && !(bApplyPending && PendingStrategyClass))
	{
		return;
	}

	TSubclassOf<UWaveStrategy> Class = PendingStrategyClass;
	PendingStrategyClass = nullptr;
	if (!Class)
	{
		Class = GetDefault<UWaveDirectorSettings>()->StrategyClass.LoadSynchronous();
	}
	if (!Class || Class->HasAnyClassFlags(CLASS_Abstract))
	{
		Class = UBudgetWaveStrategy::StaticClass();
	}

	Strategy = NewObject<UWaveStrategy>(this, Class);
	Strategy->ResetStrategy(FMath::Rand());
	UE_LOG(LogWaveDirector, Warning, TEXT("Wave strategy: %s"), *Strategy->GetStrategyName());
}

void UWaveDirectorSubsystem::SetStrategyClass(TSubclassOf<UWaveStrategy> NewClass)
{
	if (!NewClass || NewClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return;
	}
	PendingStrategyClass = NewClass;
	const FString Name = GetDefault<UWaveStrategy>(NewClass)->GetStrategyName();
	UE_LOG(LogWaveDirector, Warning, TEXT("Wave strategy will switch to %s at the next wave"), *Name);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Orange, FString::Printf(TEXT("Next wave uses strategy: %s"), *Name));
	}
}

void UWaveDirectorSubsystem::SkipIntermission()
{
	if (Phase == EPhase::Intermission)
	{
		PhaseEndTime = 0.f;
	}
}

void UWaveDirectorSubsystem::SuppressBlueprintSpawner(AActor* Spawner)
{
	if (!bSpawnerStopped)
	{
		WaveDirectorPrivate::CallNoArg(Spawner, FName(TEXT("StopSpawning")));
		bSpawnerStopped = true;
		UE_LOG(LogWaveDirector, Warning, TEXT("BP_EnemySpawner timer stopped; waves are now procedural."));
	}

	// The GameManager can restart the Blueprint timer later, so keep it cleared.
	FStructProperty* HandleProp = FindFProperty<FStructProperty>(Spawner->GetClass(), FName(TEXT("SpawnTimerHandle")));
	if (HandleProp && HandleProp->Struct == FTimerHandle::StaticStruct())
	{
		FTimerHandle* Handle = HandleProp->ContainerPtrToValuePtr<FTimerHandle>(Spawner);
		FTimerManager& Timers = GetWorld()->GetTimerManager();
		if (Handle && Timers.IsTimerActive(*Handle))
		{
			Timers.ClearTimer(*Handle);
		}
	}
}

FWaveLaneInfo* UWaveDirectorSubsystem::FindLane(int32 PathIndex)
{
	return Lanes.FindByPredicate([PathIndex](const FWaveLaneInfo& Lane) { return Lane.PathIndex == PathIndex; });
}

void UWaveDirectorSubsystem::RefreshLanes(AActor* Spawner)
{
	UWorld* World = GetWorld();
	TArray<FWaveLaneInfo> Found;

	if (SpawnPointClass)
	{
		for (TActorIterator<AActor> It(World, SpawnPointClass); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}
			const int32 PathIndex = FMath::RoundToInt(UnitReflection::GetNumeric(*It, FName(TEXT("PathIndex")), static_cast<float>(Found.Num())));
			if (Found.ContainsByPredicate([PathIndex](const FWaveLaneInfo& Lane) { return Lane.PathIndex == PathIndex; }))
			{
				continue;
			}
			FWaveLaneInfo& Lane = Found.AddDefaulted_GetRef();
			Lane.PathIndex = PathIndex;
			Lane.SpawnLocation = It->GetActorLocation();
		}
	}

	// No spawn point actors: fall back to the path count from the PathManager.
	if (Found.Num() == 0)
	{
		UObject* PathManager = nullptr;
		if (const FObjectProperty* Prop = FindFProperty<FObjectProperty>(Spawner->GetClass(), FName(TEXT("PathManager"))))
		{
			PathManager = Prop->GetObjectPropertyValue_InContainer(Spawner);
		}
		const int32 PathCount = WaveDirectorPrivate::CallIntGetter(PathManager, FName(TEXT("GetPathCount")), 0);
		for (int32 PathIndex = 0; PathIndex < PathCount; ++PathIndex)
		{
			FWaveLaneInfo& Lane = Found.AddDefaulted_GetRef();
			Lane.PathIndex = PathIndex;
			Lane.SpawnLocation = Spawner->GetActorLocation();
		}
	}

	Found.Sort([](const FWaveLaneInfo& A, const FWaveLaneInfo& B) { return A.PathIndex < B.PathIndex; });

	const AActor* Tower = GetTower();
	for (FWaveLaneInfo& Lane : Found)
	{
		if (const FWaveLaneInfo* Old = FindLane(Lane.PathIndex))
		{
			Lane.AverageProgress = Old->AverageProgress;
			Lane.EnemiesSent = Old->EnemiesSent;
			Lane.DefenderCoverage = Old->DefenderCoverage;
		}
		if (Tower)
		{
			Lane.DistanceToTower = static_cast<float>(FVector::Dist2D(Lane.SpawnLocation, Tower->GetActorLocation()));
		}
	}

	if (Found.Num() != Lanes.Num())
	{
		UE_LOG(LogWaveDirector, Log, TEXT("WaveDirector lanes: %d"), Found.Num());
	}
	Lanes = MoveTemp(Found);
}

void UWaveDirectorSubsystem::RefreshLaneCoverage()
{
	const AActor* Tower = GetTower();
	if (!Tower || !ArcherClass)
	{
		return;
	}
	const FVector TowerLocation = Tower->GetActorLocation();

	for (FWaveLaneInfo& Lane : Lanes)
	{
		Lane.DefenderCoverage = 0.f;
	}

	for (TActorIterator<AActor> It(GetWorld(), ArcherClass); It; ++It)
	{
		AActor* Defender = *It;
		if (!WaveDirectorPrivate::IsAlive(Defender))
		{
			continue;
		}

		float Weight = 1.f;
		if (GuardianClass && Defender->IsA(GuardianClass))
		{
			Weight = 1.2f;
		}
		else if (FrostArcherClass && Defender->IsA(FrostArcherClass))
		{
			Weight = 1.1f;
		}

		// The spawn-to-tower line approximates the lane; defenders in range of it cover the lane.
		const float Range = FMath::Max(400.f, UnitReflection::GetNumeric(Defender, FName(TEXT("AttackRange")), 800.f));
		const FVector Location = Defender->GetActorLocation();
		for (FWaveLaneInfo& Lane : Lanes)
		{
			const float Distance = static_cast<float>(FMath::PointDistToSegment(Location, Lane.SpawnLocation, TowerLocation));
			if (Distance <= Range)
			{
				Lane.DefenderCoverage += Weight * (1.f - 0.5f * Distance / Range);
			}
		}
	}
}

float UWaveDirectorSubsystem::GetTowerHealthPercent() const
{
	const UActorComponent* Health = UnitReflection::FindHealthComponent(GetTower());
	if (!Health)
	{
		return 1.f;
	}
	const float Max = UnitReflection::GetNumeric(Health, FName(TEXT("MaxHealth")), 0.f);
	const float Current = UnitReflection::GetNumeric(Health, FName(TEXT("CurrentHealth")), Max);
	return Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 1.f;
}

void UWaveDirectorSubsystem::SampleTower(float Now)
{
	if (Now < NextTowerSampleTime)
	{
		return;
	}
	NextTowerSampleTime = Now + 0.25f;

	TowerSamples.Add({ Now, GetTowerHealthPercent() });
	const float Window = GetDefault<UWaveDirectorSettings>()->StressWindow;
	TowerSamples.RemoveAll([Now, Window](const FTowerSample& Sample) { return Now - Sample.Time > Window; });
}

void UWaveDirectorSubsystem::UpdateTrackedEnemies()
{
	const AActor* Tower = GetTower();
	const FVector TowerLocation = Tower ? Tower->GetActorLocation() : FVector::ZeroVector;

	for (int32 Index = TrackedEnemies.Num() - 1; Index >= 0; --Index)
	{
		FTrackedEnemy& Tracked = TrackedEnemies[Index];
		AActor* Enemy = Tracked.Actor.Get();
		if (WaveDirectorPrivate::IsAlive(Enemy))
		{
			Tracked.LastLocation = Enemy->GetActorLocation();
			continue;
		}

		// 0 = died at its spawn point, 1 = died at the tower.
		float Progress = 0.5f;
		if (Tower && Tracked.SpawnDistance > 1.f)
		{
			Progress = FMath::Clamp(1.f - static_cast<float>(FVector::Dist2D(Tracked.LastLocation, TowerLocation)) / Tracked.SpawnDistance, 0.f, 1.f);
		}
		if (Tracked.Wave == CurrentWave)
		{
			++WaveKilled;
			WaveProgressSum += Progress;
		}
		if (FWaveLaneInfo* Lane = FindLane(Tracked.PathIndex))
		{
			Lane->AverageProgress = FMath::Lerp(Lane->AverageProgress, Progress, 0.25f);
		}
		TrackedEnemies.RemoveAtSwap(Index);
	}

	// Enemies spawned by anything else (e.g. the Blueprint timer before it was stopped) still count.
	if (GoblinClass)
	{
		TSet<const AActor*> Known;
		for (const FTrackedEnemy& Tracked : TrackedEnemies)
		{
			Known.Add(Tracked.Actor.Get());
		}
		for (TActorIterator<AActor> It(GetWorld(), GoblinClass); It; ++It)
		{
			if (WaveDirectorPrivate::IsAlive(*It) && !Known.Contains(*It))
			{
				FTrackedEnemy& Tracked = TrackedEnemies.AddDefaulted_GetRef();
				Tracked.Actor = *It;
				Tracked.Wave = CurrentWave;
				Tracked.LastLocation = It->GetActorLocation();
				Tracked.SpawnDistance = Tower ? static_cast<float>(FVector::Dist2D(Tracked.LastLocation, TowerLocation)) : 0.f;
			}
		}
	}
}

FWaveContext UWaveDirectorSubsystem::BuildContext(int32 WaveNumber) const
{
	FWaveContext Context;
	Context.WaveNumber = WaveNumber;
	Context.TowerHealthPercent = GetTowerHealthPercent();
	Context.Lanes = Lanes;
	Context.EnemiesAlive = TrackedEnemies.Num();

	float HighestRecent = Context.TowerHealthPercent;
	for (const FTowerSample& Sample : TowerSamples)
	{
		HighestRecent = FMath::Max(HighestRecent, Sample.HealthPercent);
	}
	Context.RecentTowerDamage = FMath::Max(0.f, HighestRecent - Context.TowerHealthPercent);

	Context.Gold = UnitReflection::GetNumeric(CachedGameManager.Get(), FName(TEXT("Gold")), 0.f);

	if (ArcherClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), ArcherClass); It; ++It)
		{
			if (!WaveDirectorPrivate::IsAlive(*It))
			{
				continue;
			}
			if (GuardianClass && It->IsA(GuardianClass))
			{
				++Context.NumGuardians;
			}
			else if (FrostArcherClass && It->IsA(FrostArcherClass))
			{
				++Context.NumFrostArchers;
			}
			else
			{
				++Context.NumArchers;
			}
		}
	}

	if (const AActor* Tower = GetTower())
	{
		const float RadiusSq = FMath::Square(GetDefault<UWaveDirectorSettings>()->NearTowerRadius);
		for (const FTrackedEnemy& Tracked : TrackedEnemies)
		{
			if (FVector::DistSquared2D(Tracked.LastLocation, Tower->GetActorLocation()) <= RadiusSq)
			{
				++Context.EnemiesNearTower;
			}
		}
	}
	return Context;
}

void UWaveDirectorSubsystem::BeginIntermission(float Now)
{
	Phase = EPhase::Intermission;
	PhaseEndTime = Now + Strategy->GetIntermissionDuration(CurrentWave + 1);
}

void UWaveDirectorSubsystem::BeginWave(float Now)
{
	if (Lanes.Num() == 0)
	{
		UE_LOG(LogWaveDirector, Warning, TEXT("WaveDirector: no lanes yet, waiting for spawn points to register."));
		PhaseEndTime = Now + 1.f;
		return;
	}

	EnsureStrategy(true);

	++CurrentWave;
	LastContext = BuildContext(CurrentWave);
	CurrentPlan = Strategy->BuildWave(LastContext);
	for (FWaveSpawnEntry& Entry : CurrentPlan.Spawns)
	{
		Entry.LaneIndex = FMath::Clamp(Entry.LaneIndex, 0, LastContext.Lanes.Num() - 1);
	}
	LastPlanSummary = CurrentPlan.Summary;

	SpawnCursor = 0;
	WaveSpawned = 0;
	WaveKilled = 0;
	WaveProgressSum = 0.f;
	WaveStartTime = Now;
	WaveStartTowerHealth = GetTowerHealthPercent();
	NextSpawnTime = Now + (CurrentPlan.Spawns.Num() > 0 ? CurrentPlan.Spawns[0].Delay : 0.f);
	Phase = EPhase::Spawning;

	UE_LOG(LogWaveDirector, Warning, TEXT("=== WAVE %d (%s): %s"), CurrentWave, *Strategy->GetStrategyName(), *CurrentPlan.Summary);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("WAVE %d incoming!"), CurrentWave));
	}
}

void UWaveDirectorSubsystem::TickSpawning(float Now)
{
	if (SpawnCursor >= CurrentPlan.Spawns.Num())
	{
		Phase = EPhase::Clearing;
		LastSpawnTime = Now;
		return;
	}
	if (Now < NextSpawnTime)
	{
		return;
	}

	SpawnEnemy(CachedSpawner.Get(), CurrentPlan.Spawns[SpawnCursor]);
	++SpawnCursor;
	LastSpawnTime = Now;

	if (SpawnCursor < CurrentPlan.Spawns.Num())
	{
		LastContext = BuildContext(CurrentWave);
		const float Pacing = Strategy->GetPacingScale(LastContext);
		NextSpawnTime = Now + CurrentPlan.Spawns[SpawnCursor].Delay * Pacing;
	}
	else
	{
		Phase = EPhase::Clearing;
	}
}

void UWaveDirectorSubsystem::FinishWave(float Now)
{
	FWaveReport Report;
	Report.WaveNumber = CurrentWave;
	Report.EnemiesSpawned = WaveSpawned;
	Report.EnemiesKilled = WaveKilled;
	Report.Duration = Now - WaveStartTime;
	Report.TowerDamageTaken = FMath::Max(0.f, WaveStartTowerHealth - GetTowerHealthPercent());
	Report.AverageProgress = WaveKilled > 0 ? WaveProgressSum / WaveKilled : 0.f;
	Report.GoldAtEnd = UnitReflection::GetNumeric(CachedGameManager.Get(), FName(TEXT("Gold")), 0.f);

	Strategy->OnWaveFinished(Report);

	UE_LOG(LogWaveDirector, Warning, TEXT("=== WAVE %d CLEARED in %.0fs: %d/%d killed, tower -%.0f%%, avg progress %.2f"),
		Report.WaveNumber, Report.Duration, Report.EnemiesKilled, Report.EnemiesSpawned, Report.TowerDamageTaken * 100.f, Report.AverageProgress);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, FString::Printf(TEXT("Wave %d cleared!"), CurrentWave));
	}

	BeginIntermission(Now);
}

UClass* UWaveDirectorSubsystem::GetEnemyClass(EWaveEnemyType Type) const
{
	switch (Type)
	{
	case EWaveEnemyType::Berserker:
		return BerserkerClass ? BerserkerClass.Get() : GoblinClass.Get();
	case EWaveEnemyType::Shaman:
		return ShamanClass ? ShamanClass.Get() : GoblinClass.Get();
	default:
		return GoblinClass;
	}
}

AActor* UWaveDirectorSubsystem::CallSpawnEnemyOnPath(AActor* Spawner, int32 PathIndex) const
{
	UFunction* Fn = Spawner ? Spawner->FindFunction(FName(TEXT("SpawnEnemyOnPath"))) : nullptr;
	if (!Fn || Fn->ParmsSize == 0)
	{
		return nullptr;
	}

	uint8* Parms = static_cast<uint8*>(FMemory_Alloca_Aligned(Fn->ParmsSize, Fn->GetMinAlignment()));
	FMemory::Memzero(Parms, Fn->ParmsSize);
	for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Parms);
	}

	// The first int input is the path index.
	for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			if (FIntProperty* IntProp = CastField<FIntProperty>(*It))
			{
				IntProp->SetPropertyValue_InContainer(Parms, PathIndex);
				break;
			}
		}
	}

	Spawner->ProcessEvent(Fn, Parms);

	AActor* Spawned = nullptr;
	for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			if (FObjectProperty* ObjProp = CastField<FObjectProperty>(*It))
			{
				Spawned = Cast<AActor>(ObjProp->GetObjectPropertyValue_InContainer(Parms));
				break;
			}
		}
	}

	for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Parms);
	}
	return Spawned;
}

AActor* UWaveDirectorSubsystem::SpawnEnemy(AActor* Spawner, const FWaveSpawnEntry& Entry)
{
	if (!IsValid(Spawner) || !LastContext.Lanes.IsValidIndex(Entry.LaneIndex))
	{
		return nullptr;
	}
	const int32 PathIndex = LastContext.Lanes[Entry.LaneIndex].PathIndex;

	// The Blueprint spawns whatever GoblinClass holds, so point it at the planned type for this spawn.
	FClassProperty* ClassProp = FindFProperty<FClassProperty>(Spawner->GetClass(), FName(TEXT("GoblinClass")));
	if (ClassProp)
	{
		ClassProp->SetObjectPropertyValue_InContainer(Spawner, GetEnemyClass(Entry.Type));
	}

	AActor* Enemy = CallSpawnEnemyOnPath(Spawner, PathIndex);
	if (!IsValid(Enemy) && !WaveDirectorPrivate::GetBool(Spawner, FName(TEXT("bEnableSpawning")), true))
	{
		WaveDirectorPrivate::SetBool(Spawner, FName(TEXT("bEnableSpawning")), true);
		Enemy = CallSpawnEnemyOnPath(Spawner, PathIndex);
	}

	if (ClassProp)
	{
		ClassProp->SetObjectPropertyValue_InContainer(Spawner, GoblinClass);
	}

	if (!IsValid(Enemy))
	{
		UE_LOG(LogWaveDirector, Warning, TEXT("SpawnEnemyOnPath failed (%s on path %d)"), UWaveStrategy::GetEnemyTypeName(Entry.Type), PathIndex);
		return nullptr;
	}

	// Variant components set their own max health on BeginPlay, so attach them before scaling.
	if (UUnitRosterSubsystem* Roster = GetWorld()->GetSubsystem<UUnitRosterSubsystem>())
	{
		Roster->ApplyVariantComponents(Enemy);
	}
	ApplyModifiers(Enemy, Entry);

	if (FWaveLaneInfo* Lane = FindLane(PathIndex))
	{
		++Lane->EnemiesSent;
	}
	++WaveSpawned;

	const AActor* Tower = GetTower();
	FTrackedEnemy& Tracked = TrackedEnemies.AddDefaulted_GetRef();
	Tracked.Actor = Enemy;
	Tracked.Wave = CurrentWave;
	Tracked.PathIndex = PathIndex;
	Tracked.LastLocation = Enemy->GetActorLocation();
	Tracked.SpawnDistance = Tower ? static_cast<float>(FVector::Dist2D(Tracked.LastLocation, Tower->GetActorLocation())) : 0.f;
	return Enemy;
}

void UWaveDirectorSubsystem::ApplyModifiers(AActor* Enemy, const FWaveSpawnEntry& Entry) const
{
	const FName MaxHealthName(TEXT("MaxHealth"));
	const FName CurrentHealthName(TEXT("CurrentHealth"));

	if (!FMath::IsNearlyEqual(Entry.HealthMultiplier, 1.f))
	{
		if (UActorComponent* Health = UnitReflection::FindHealthComponent(Enemy))
		{
			const float NewMax = UnitReflection::GetNumeric(Health, MaxHealthName, 100.f) * Entry.HealthMultiplier;
			UnitReflection::SetNumeric(Health, MaxHealthName, NewMax);
			UnitReflection::SetNumeric(Health, CurrentHealthName, NewMax);
			if (UnitReflection::HasNumeric(Enemy, CurrentHealthName))
			{
				UnitReflection::SetNumeric(Enemy, MaxHealthName, NewMax);
				UnitReflection::SetNumeric(Enemy, CurrentHealthName, NewMax);
			}
			WaveDirectorPrivate::CallNoArg(Health, FName(TEXT("UpdateHealthDisplay")));
		}
	}

	if (!FMath::IsNearlyEqual(Entry.SpeedMultiplier, 1.f))
	{
		const FName MoveSpeedName(TEXT("MoveSpeed"));
		UnitReflection::SetNumeric(Enemy, MoveSpeedName, UnitReflection::GetNumeric(Enemy, MoveSpeedName, 0.f) * Entry.SpeedMultiplier);
	}

	if (!FMath::IsNearlyEqual(Entry.GoldMultiplier, 1.f))
	{
		const FName GoldName(TEXT("GoldReward"));
		UnitReflection::SetNumeric(Enemy, GoldName, UnitReflection::GetNumeric(Enemy, GoldName, 25.f) * Entry.GoldMultiplier);
	}
}

FString UWaveDirectorSubsystem::GetStrategyName() const
{
	return Strategy ? Strategy->GetStrategyName() : FString(TEXT("None"));
}

int32 UWaveDirectorSubsystem::GetEnemiesRemaining() const
{
	const int32 Unspawned = Phase == EPhase::Spawning ? FMath::Max(0, CurrentPlan.Spawns.Num() - SpawnCursor) : 0;
	return TrackedEnemies.Num() + Unspawned;
}

FString UWaveDirectorSubsystem::GetStatusText() const
{
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	switch (Phase)
	{
	case EPhase::Intermission:
		return FString::Printf(TEXT("Next wave in %ds"), FMath::CeilToInt(FMath::Max(0.f, PhaseEndTime - Now)));
	case EPhase::Spawning:
		return FString::Printf(TEXT("Spawning %d/%d | Enemies left: %d"), SpawnCursor, CurrentPlan.Spawns.Num(), GetEnemiesRemaining());
	case EPhase::Clearing:
		return FString::Printf(TEXT("Enemies left: %d"), GetEnemiesRemaining());
	default:
		return TEXT("Waiting for game");
	}
}

void UWaveDirectorSubsystem::DrawDebugHud() const
{
	if (!GEngine)
	{
		return;
	}
	using namespace WaveDirectorPrivate;

	FString LaneText;
	for (const FWaveLaneInfo& Lane : Lanes)
	{
		LaneText += FString::Printf(TEXT("  [path %d: progress %.2f, cover %.1f]"), Lane.PathIndex, Lane.AverageProgress, Lane.DefenderCoverage);
	}

	GEngine->AddOnScreenDebugMessage(HudKeyBase + 4, 0.5f, FColor::Silver, FString::Printf(TEXT("Lanes:%s"), *LaneText));
	GEngine->AddOnScreenDebugMessage(HudKeyBase + 3, 0.5f, FColor::Cyan, Strategy ? Strategy->GetDebugStatus() : FString());
	if (!LastPlanSummary.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(HudKeyBase + 2, 0.5f, FColor::White, LastPlanSummary);
	}
	GEngine->AddOnScreenDebugMessage(HudKeyBase + 1, 0.5f, FColor::Yellow,
		FString::Printf(TEXT("WAVE %d | %s | Strategy: %s"), CurrentWave, *GetStatusText(), *GetStrategyName()));
}

void UWaveDirectorSubsystem::Tick(float DeltaTime)
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

	const UWaveDirectorSettings* Settings = GetDefault<UWaveDirectorSettings>();
	if (!Settings->bEnableWaveDirector)
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

	if (!CachedGameManager.IsValid())
	{
		CachedGameManager = FindFirstActor(GameManagerClass);
	}
	if (!IsGamePlaying(CachedGameManager.Get()))
	{
		return;
	}

	if (!CachedSpawner.IsValid())
	{
		CachedSpawner = FindFirstActor(SpawnerClass);
		bSpawnerStopped = false;
	}
	AActor* Spawner = CachedSpawner.Get();
	if (!Spawner)
	{
		return;
	}
	SuppressBlueprintSpawner(Spawner);
	EnsureStrategy(false);

	if (Now >= NextLaneRefreshTime)
	{
		RefreshLanes(Spawner);
		RefreshLaneCoverage();
		NextLaneRefreshTime = Now + 1.f;
	}
	SampleTower(Now);
	UpdateTrackedEnemies();

	if (!bDirecting)
	{
		bDirecting = true;
		BeginIntermission(Now);
	}

	switch (Phase)
	{
	case EPhase::Intermission:
		if (Now >= PhaseEndTime)
		{
			BeginWave(Now);
		}
		break;
	case EPhase::Spawning:
		TickSpawning(Now);
		break;
	case EPhase::Clearing:
		if (TrackedEnemies.Num() == 0 || Now - LastSpawnTime > Settings->ClearTimeout)
		{
			FinishWave(Now);
		}
		break;
	default:
		break;
	}

	if (Settings->bShowDebugHud)
	{
		DrawDebugHud();
	}
}
