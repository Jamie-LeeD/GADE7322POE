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
