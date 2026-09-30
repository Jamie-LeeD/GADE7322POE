#pragma once

#include "CoreMinimal.h"
#include "WaveTypes.generated.h"

UENUM(BlueprintType)
enum class EWaveEnemyType : uint8
{
	Goblin,
	Berserker,
	Shaman
};

/** One enemy lane: a spawn point and the path it feeds, plus what the director has learned about it. */
USTRUCT(BlueprintType)
struct FWaveLaneInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 PathIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	FVector SpawnLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float DistanceToTower = 0.f;

	/** Weighted number of defenders covering the straight line from this spawn point to the tower. */
	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float DefenderCoverage = 0.f;

	/** Smoothed 0..1 value of how far enemies on this lane got before dying (1 = reached the tower). */
	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float AverageProgress = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 EnemiesSent = 0;
};

/** Snapshot of the game and the player that strategies use to build and pace waves. */
USTRUCT(BlueprintType)
struct FWaveContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 WaveNumber = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float TowerHealthPercent = 1.f;

	/** Fraction of the tower's max health lost during the recent stress window. */
	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float RecentTowerDamage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float Gold = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 NumArchers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 NumFrostArchers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 NumGuardians = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 EnemiesAlive = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 EnemiesNearTower = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	TArray<FWaveLaneInfo> Lanes;

	int32 GetDefenderCount() const { return NumArchers + NumFrostArchers + NumGuardians; }
};

USTRUCT(BlueprintType)
struct FWaveSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	EWaveEnemyType Type = EWaveEnemyType::Goblin;

	/** Index into FWaveContext::Lanes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	int32 LaneIndex = 0;

	/** Seconds to wait after the previous spawn (or after the wave starts, for the first entry). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	float Delay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	float HealthMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	float SpeedMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	float GoldMultiplier = 1.f;
};

USTRUCT(BlueprintType)
struct FWavePlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	TArray<FWaveSpawnEntry> Spawns;

	/** Short human-readable description shown on the debug HUD and in the log. */
	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	FString Summary;
};

/** What happened during a finished wave, fed back into the strategy so it can adapt. */
USTRUCT(BlueprintType)
struct FWaveReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 WaveNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 EnemiesSpawned = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 EnemiesKilled = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float Duration = 0.f;

	/** Fraction of the tower's max health lost during the wave. */
	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float TowerDamageTaken = 0.f;

	/** Average 0..1 distance enemies covered before dying. */
	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float AverageProgress = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	float GoldAtEnd = 0.f;
};
