#include "CounterWaveStrategy.h"

DEFINE_LOG_CATEGORY_STATIC(LogCounterWaves, Log, All);

void UCounterWaveStrategy::ResetStrategy(int32 Seed)
{
	Super::ResetStrategy(Seed);
	Momentum = 1.f;
	LastBerserkerShare = 0.f;
	LastShamanShare = 0.f;
	LastGreedEnemies = 0;
	LastStress = 0.f;
}

float UCounterWaveStrategy::GetIntensity(int32 Wave) const
{
	if (IntensityPattern.Num() == 0)
	{
		return 1.f;
	}
	return IntensityPattern[(FMath::Max(1, Wave) - 1) % IntensityPattern.Num()];
}

bool UCounterWaveStrategy::IsPeakWave(int32 Wave) const
{
	float Highest = 0.f;
	for (const float Value : IntensityPattern)
	{
		Highest = FMath::Max(Highest, Value);
	}
	return IntensityPattern.Num() > 1 && FMath::IsNearlyEqual(GetIntensity(Wave), Highest);
}

float UCounterWaveStrategy::ComputeStress(const FWaveContext& Context) const
{
	return Context.RecentTowerDamage / StressDamageReference + Context.EnemiesNearTower * StressPerEnemyNearTower;
}

void UCounterWaveStrategy::ComputeCounterShares(const FWaveContext& Context, float& OutBerserker, float& OutShaman) const
{
	const float Defenders = static_cast<float>(Context.GetDefenderCount());
	const float ArcherFraction = Defenders > 0.f ? Context.NumArchers / Defenders : 0.f;
	const float FrostFraction = Defenders > 0.f ? Context.NumFrostArchers / Defenders : 0.f;
	const float GuardianFraction = Defenders > 0.f ? Context.NumGuardians / Defenders : 0.f;

	// Fast raging Berserkers rush through archer fire; ranged, buffing Shamans out-range melee
	// Guardians and their speed buff cancels Frost Archer slows.
	OutBerserker = Context.WaveNumber >= BerserkerUnlockWave ? BaseBerserkerShare + BerserkerVsArchers * ArcherFraction : 0.f;
	OutShaman = Context.WaveNumber >= ShamanUnlockWave ? BaseShamanShare + ShamanVsGuardians * GuardianFraction + ShamanVsFrost * FrostFraction : 0.f;

	const float MaxSpecial = 1.f - MinGoblinShare;
	const float Special = OutBerserker + OutShaman;
	if (Special > MaxSpecial && Special > 0.f)
	{
		OutBerserker *= MaxSpecial / Special;
		OutShaman *= MaxSpecial / Special;
	}
}

TArray<int32> UCounterWaveStrategy::RankLanesByWeakness(const FWaveContext& Context) const
{
	TArray<int32> Order;
	for (int32 Index = 0; Index < Context.Lanes.Num(); ++Index)
	{
		Order.Add(Index);
	}
	Order.Sort([&Context](int32 A, int32 B)
	{
		return Context.Lanes[A].DefenderCoverage < Context.Lanes[B].DefenderCoverage;
	});
	return Order;
}

FWavePlan UCounterWaveStrategy::BuildWave(const FWaveContext& Context)
{
	const int32 Wave = Context.WaveNumber;
	const float Intensity = GetIntensity(Wave);
	const bool bPeak = IsPeakWave(Wave);

	// Enemy count: rising baseline shaped by the intensity curve and last wave's momentum.
	const float Baseline = BaseEnemies + EnemiesPerWave * (Wave - 1);
	int32 Count = FMath::Max(1, FMath::RoundToInt(Baseline * Intensity * Momentum));

	LastGreedEnemies = 0;
	if (Wave >= GreedStartWave && Context.Gold > GreedThreshold)
	{
		LastGreedEnemies = FMath::Min(MaxGreedEnemies, FMath::FloorToInt((Context.Gold - GreedThreshold) / GoldPerGreedEnemy) + 1);
		Count += LastGreedEnemies;
	}

	// Past the cap, extra difficulty comes from health instead of more bodies.
	float HealthMultiplier = 1.f;
	if (Count > MaxEnemiesPerWave)
	{
		HealthMultiplier += static_cast<float>(Count - MaxEnemiesPerWave) / MaxEnemiesPerWave;
		Count = MaxEnemiesPerWave;
	}
	if (Wave >= HealthScalingStartWave)
	{
		HealthMultiplier += HealthPerWave * (Wave - HealthScalingStartWave + 1);
	}

	ComputeCounterShares(Context, LastBerserkerShare, LastShamanShare);

	// Exact counts from the shares so the mix is readable, then shuffle the front line.
	const int32 Shamans = FMath::RoundToInt(Count * LastShamanShare);
	const int32 Berserkers = FMath::Min(Count - Shamans, FMath::RoundToInt(Count * LastBerserkerShare));
	const int32 Goblins = Count - Shamans - Berserkers;

	TArray<EWaveEnemyType> FrontLine;
	FrontLine.Init(EWaveEnemyType::Goblin, Goblins);
	for (int32 Index = 0; Index < Berserkers; ++Index)
	{
		FrontLine.Add(EWaveEnemyType::Berserker);
	}
	for (int32 Index = FrontLine.Num() - 1; Index > 0; --Index)
	{
		FrontLine.Swap(Index, Random.RandRange(0, Index));
	}

	TArray<TArray<EWaveEnemyType>> Squads;
	for (int32 Index = 0; Index < FrontLine.Num();)
	{
		TArray<EWaveEnemyType>& Squad = Squads.AddDefaulted_GetRef();
		const int32 Size = Random.RandRange(MinSquadSize, MaxSquadSize);
		for (int32 Slot = 0; Slot < Size && Index < FrontLine.Num(); ++Slot)
		{
			Squad.Add(FrontLine[Index++]);
		}
	}
	if (Squads.Num() == 0)
	{
		Squads.AddDefaulted();
	}
	for (int32 Index = 0; Index < Shamans; ++Index)
	{
		Squads[Index % Squads.Num()].Add(EWaveEnemyType::Shaman);
	}

	// Lanes: normal waves lean on the weakest lane, peak waves hit every lane at once.
	const TArray<int32> LaneRanking = RankLanesByWeakness(Context);
	const int32 LaneCount = FMath::Max(1, LaneRanking.Num());
	const float SquadGap = FMath::Max(MinSquadGap, BaseSquadGap - 0.15f * (Wave - 1));

	FWavePlan Plan;
	for (int32 SquadIndex = 0; SquadIndex < Squads.Num(); ++SquadIndex)
	{
		int32 Lane = 0;
		if (LaneRanking.Num() > 0)
		{
			if (bPeak)
			{
				Lane = LaneRanking[SquadIndex % LaneCount];
			}
			else if (Random.FRand() < WeakestLaneShare || LaneCount == 1)
			{
				Lane = LaneRanking[0];
			}
			else
			{
				Lane = LaneRanking[Random.RandRange(1, LaneCount - 1)];
			}
		}

		const TArray<EWaveEnemyType>& Squad = Squads[SquadIndex];
		for (int32 Slot = 0; Slot < Squad.Num(); ++Slot)
		{
			FWaveSpawnEntry& Entry = Plan.Spawns.AddDefaulted_GetRef();
			Entry.Type = Squad[Slot];
			Entry.LaneIndex = Lane;
			Entry.HealthMultiplier = HealthMultiplier;
			if (Plan.Spawns.Num() == 1)
			{
				Entry.Delay = 0.5f;
			}
			else if (Slot == 0)
			{
				// Peak waves launch squads on every lane almost together (a pincer).
				Entry.Delay = bPeak && SquadIndex % LaneCount != 0 ? InSquadInterval : SquadGap;
			}
			else
			{
				Entry.Delay = InSquadInterval;
			}
		}
	}

	const FString WeakLane = LaneRanking.Num() > 0 ? FString::Printf(TEXT("path %d"), Context.Lanes[LaneRanking[0]].PathIndex) : TEXT("none");
	Plan.Summary = FString::Printf(TEXT("Intensity %.2f%s -> %d Goblin, %d Berserker, %d Shaman (+%d greed), HP x%.2f, weakest lane %s"),
		Intensity, bPeak ? TEXT(" [PEAK/PINCER]") : TEXT(""), Goblins, Berserkers, Shamans, LastGreedEnemies, HealthMultiplier, *WeakLane);

	UE_LOG(LogCounterWaves, Log, TEXT("Wave %d: %s | defenders A%d F%d G%d | momentum %.2f"),
		Wave, *Plan.Summary, Context.NumArchers, Context.NumFrostArchers, Context.NumGuardians, Momentum);
	return Plan;
}

void UCounterWaveStrategy::OnWaveFinished(const FWaveReport& Report)
{
	// A clean wave pushes the next one up, a painful wave eases it off.
	if (Report.TowerDamageTaken <= 0.01f && Report.AverageProgress < 0.5f)
	{
		Momentum += 0.1f;
	}
	else if (Report.TowerDamageTaken >= 0.25f)
	{
		Momentum -= 0.2f;
	}
	else if (Report.TowerDamageTaken >= 0.1f)
	{
		Momentum -= 0.1f;
	}
	Momentum = FMath::Clamp(Momentum, 0.6f, 1.6f);

	UE_LOG(LogCounterWaves, Log, TEXT("Wave %d report: tower dmg %.0f%%, progress %.2f -> momentum %.2f"),
		Report.WaveNumber, Report.TowerDamageTaken * 100.f, Report.AverageProgress, Momentum);
}

float UCounterWaveStrategy::GetPacingScale(const FWaveContext& Context) const
{
	LastStress = ComputeStress(Context);
	if (LastStress >= 1.f)
	{
		return HighStressPacing;
	}
	if (LastStress <= 0.2f && Context.EnemiesAlive <= 3)
	{
		return LowStressPacing;
	}
	return FMath::GetMappedRangeValueClamped(FVector2f(0.2f, 1.f), FVector2f(1.f, HighStressPacing), LastStress);
}

FString UCounterWaveStrategy::GetDebugStatus() const
{
	return FString::Printf(TEXT("Stress %.2f | Momentum x%.2f | Counter mix: Berserker %.0f%% Shaman %.0f%% | Greed +%d"),
		LastStress, Momentum, LastBerserkerShare * 100.f, LastShamanShare * 100.f, LastGreedEnemies);
}
