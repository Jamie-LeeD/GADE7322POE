#include "WaveStrategy.h"

void UWaveStrategy::ResetStrategy(int32 Seed)
{
	Random.Initialize(Seed);
}

float UWaveStrategy::GetIntermissionDuration(int32 UpcomingWave) const
{
	return UpcomingWave <= 1 ? FirstIntermission : Intermission;
}

const TCHAR* UWaveStrategy::GetEnemyTypeName(EWaveEnemyType Type)
{
	switch (Type)
	{
	case EWaveEnemyType::Berserker:
		return TEXT("Berserker");
	case EWaveEnemyType::Shaman:
		return TEXT("Shaman");
	default:
		return TEXT("Goblin");
	}
}

int32 UWaveStrategy::PickWeighted(const TArray<float>& Weights)
{
	float Total = 0.f;
	for (const float Weight : Weights)
	{
		Total += FMath::Max(0.f, Weight);
	}
	if (Total <= 0.f)
	{
		return 0;
	}

	float Roll = Random.FRandRange(0.f, Total);
	int32 LastPositive = 0;
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		const float Weight = FMath::Max(0.f, Weights[Index]);
		if (Weight <= 0.f)
		{
			continue;
		}
		if (Roll <= Weight)
		{
			return Index;
		}
		Roll -= Weight;
		LastPositive = Index;
	}
	return LastPositive;
}

void UWaveStrategy::AssignEliteSpawns(FWavePlan& Plan, int32 WaveNumber, bool bGuaranteeElite, float EliteChance)
{
	TArray<int32> GoblinIndices;
	for (int32 Index = 0; Index < Plan.Spawns.Num(); ++Index)
	{
		if (Plan.Spawns[Index].Type == EWaveEnemyType::Goblin && !Plan.Spawns[Index].bElite)
		{
			GoblinIndices.Add(Index);
		}
	}
	if (GoblinIndices.Num() == 0)
	{
		return;
	}

	const bool bRollElite = bGuaranteeElite || Random.FRand() < FMath::Clamp(EliteChance, 0.f, 1.f);
	if (!bRollElite)
	{
		return;
	}

	const int32 Pick = GoblinIndices[Random.RandRange(0, GoblinIndices.Num() - 1)];
	FWaveSpawnEntry& Entry = Plan.Spawns[Pick];
	Entry.bElite = true;
	Entry.EliteSeed = (WaveNumber * 73856093) ^ ((Pick + 1) * 19349663) ^ Random.RandHelper(0x7fffffff);
}
