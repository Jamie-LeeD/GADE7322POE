#include "Waves/BudgetWaveStrategy.h"

DEFINE_LOG_CATEGORY_STATIC(LogBudgetWaves, Log, All);

void UBudgetWaveStrategy::ResetStrategy(int32 Seed)
{
	Super::ResetStrategy(Seed);
	SkillRating = 1.f;
	LastPerformance = 0.f;
	LastBudget = 0.f;
}

float UBudgetWaveStrategy::GetEnemyCost(EWaveEnemyType Type) const
{
	switch (Type)
	{
	case EWaveEnemyType::Berserker:
		return BerserkerCost;
	case EWaveEnemyType::Shaman:
		return ShamanCost;
	default:
		return GoblinCost;
	}
}

bool UBudgetWaveStrategy::IsMilestoneWave(int32 Wave) const
{
	return MilestoneEvery > 0 && Wave % MilestoneEvery == 0;
}

bool UBudgetWaveStrategy::IsBreatherWave(int32 Wave) const
{
	return Wave > 1 && IsMilestoneWave(Wave - 1);
}

float UBudgetWaveStrategy::ComputeBudget(int32 Wave) const
{
	const float WaveIndex = static_cast<float>(FMath::Max(0, Wave - 1));
	float Budget = (BaseBudget + BudgetPerWave * WaveIndex) * FMath::Pow(BudgetGrowth, WaveIndex);
	Budget *= SkillRating;
	if (IsMilestoneWave(Wave))
	{
		Budget *= MilestoneBudgetScale;
	}
	else if (IsBreatherWave(Wave))
	{
		Budget *= BreatherBudgetScale;
	}
	return Budget;
}

int32 UBudgetWaveStrategy::PickLane(const FWaveContext& Context)
{
	if (Context.Lanes.Num() <= 1)
	{
		return 0;
	}

	// Lanes where enemies got further are weaker spots in the defence, so they get more squads.
	TArray<float> Weights;
	Weights.Reserve(Context.Lanes.Num());
	for (const FWaveLaneInfo& Lane : Context.Lanes)
	{
		Weights.Add(MinLaneWeight + Lane.AverageProgress * 2.f);
	}
	return PickWeighted(Weights);
}

FWavePlan UBudgetWaveStrategy::BuildWave(const FWaveContext& Context)
{
	const int32 Wave = Context.WaveNumber;
	const float Budget = ComputeBudget(Wave);
	LastBudget = Budget;

	// Buy enemies until the budget runs out or the enemy cap is reached.
	TArray<EWaveEnemyType> Bought;
	float Remaining = Budget;
	int32 Shamans = 0;
	while (Remaining >= GoblinCost && Bought.Num() < MaxEnemiesPerWave)
	{
		TArray<EWaveEnemyType> Options;
		TArray<float> Weights;

		Options.Add(EWaveEnemyType::Goblin);
		Weights.Add(1.f);

		if (Wave >= BerserkerUnlockWave && Remaining >= BerserkerCost)
		{
			Options.Add(EWaveEnemyType::Berserker);
			Weights.Add(BerserkerWeight + BerserkerWeightPerWave * (Wave - BerserkerUnlockWave));
		}

		const int32 Others = Bought.Num() - Shamans;
		const int32 ShamanCap = Others >= 2 ? FMath::Max(1, Others / EnemiesPerShaman) : 0;
		if (Wave >= ShamanUnlockWave && Remaining >= ShamanCost && Shamans < ShamanCap)
		{
			Options.Add(EWaveEnemyType::Shaman);
			Weights.Add(ShamanWeight);
		}

		const EWaveEnemyType Choice = Options[PickWeighted(Weights)];
		Bought.Add(Choice);
		Remaining -= GetEnemyCost(Choice);
		if (Choice == EWaveEnemyType::Shaman)
		{
			++Shamans;
		}
	}

	// Budget left over after hitting the cap becomes tougher enemies rather than more enemies.
	float HealthMultiplier = 1.f;
	if (Bought.Num() >= MaxEnemiesPerWave && Budget > 0.f)
	{
		HealthMultiplier = 1.f + Remaining / Budget;
	}

	// Split the front line into squads, then put Shamans at the back of the squads so the
	// front line soaks damage for them.
	TArray<TArray<EWaveEnemyType>> Squads;
	TArray<EWaveEnemyType> Support;
	for (const EWaveEnemyType Type : Bought)
	{
		if (Type == EWaveEnemyType::Shaman)
		{
			Support.Add(Type);
			continue;
		}
		if (Squads.Num() == 0 || Squads.Last().Num() >= Random.RandRange(MinSquadSize, MaxSquadSize))
		{
			Squads.AddDefaulted();
		}
		Squads.Last().Add(Type);
	}
	if (Squads.Num() == 0)
	{
		Squads.AddDefaulted();
	}
	for (int32 Index = 0; Index < Support.Num(); ++Index)
	{
		Squads[Index % Squads.Num()].Add(Support[Index]);
	}

	const float SpawnInterval = FMath::Max(MinSpawnInterval, BaseSpawnInterval * FMath::Pow(SpawnIntervalDecay, static_cast<float>(Wave - 1)));

	FWavePlan Plan;
	for (const TArray<EWaveEnemyType>& Squad : Squads)
	{
		const int32 Lane = PickLane(Context);
		for (int32 Slot = 0; Slot < Squad.Num(); ++Slot)
		{
			FWaveSpawnEntry& Entry = Plan.Spawns.AddDefaulted_GetRef();
			Entry.Type = Squad[Slot];
			Entry.LaneIndex = Lane;
			Entry.Delay = Plan.Spawns.Num() == 1 ? 0.5f : (Slot == 0 ? SquadGap : SpawnInterval);
			Entry.HealthMultiplier = HealthMultiplier;
		}
	}

	int32 Counts[3] = { 0, 0, 0 };
	for (const EWaveEnemyType Type : Bought)
	{
		++Counts[static_cast<int32>(Type)];
	}
	const TCHAR* Tag = IsMilestoneWave(Wave) ? TEXT(" [MILESTONE]") : (IsBreatherWave(Wave) ? TEXT(" [BREATHER]") : TEXT(""));
	Plan.Summary = FString::Printf(TEXT("Budget %.1f -> %d Goblin, %d Berserker, %d Shaman, HP x%.2f%s"),
		Budget, Counts[0], Counts[1], Counts[2], HealthMultiplier, Tag);

	UE_LOG(LogBudgetWaves, Log, TEXT("Wave %d: %s (skill %.2f)"), Wave, *Plan.Summary, SkillRating);
	return Plan;
}

void UBudgetWaveStrategy::OnWaveFinished(const FWaveReport& Report)
{
	// Positive = the player handled the wave comfortably, negative = the wave was too much.
	const float DamageScore = (TargetTowerDamage - Report.TowerDamageTaken) / TargetTowerDamage;
	const float ProgressScore = (TargetProgress - Report.AverageProgress) / TargetProgress;
	LastPerformance = FMath::Clamp(0.6f * DamageScore + 0.4f * ProgressScore, -1.f, 1.f);

	SkillRating = FMath::Clamp(SkillRating + LastPerformance * SkillLearnRate, MinSkillRating, MaxSkillRating);

	UE_LOG(LogBudgetWaves, Log, TEXT("Wave %d report: tower dmg %.0f%%, progress %.2f -> performance %+.2f, skill %.2f"),
		Report.WaveNumber, Report.TowerDamageTaken * 100.f, Report.AverageProgress, LastPerformance, SkillRating);
}

float UBudgetWaveStrategy::GetIntermissionDuration(int32 UpcomingWave) const
{
	// Extra build time before a milestone wave and a longer rest after one.
	const float Base = Super::GetIntermissionDuration(UpcomingWave);
	return (IsMilestoneWave(UpcomingWave) || IsBreatherWave(UpcomingWave)) ? Base + 4.f : Base;
}

FString UBudgetWaveStrategy::GetDebugStatus() const
{
	return FString::Printf(TEXT("Skill x%.2f | Last performance %+.2f | Budget %.1f"), SkillRating, LastPerformance, LastBudget);
}
