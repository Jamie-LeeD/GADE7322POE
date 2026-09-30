#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WaveTypes.h"
#include "WaveDirectorSubsystem.generated.h"

class UWaveStrategy;

/**
 * Runs procedural enemy waves. Takes over from BP_EnemySpawner's fixed timer, measures the player
 * (tower damage, gold, defender mix, how far enemies get per lane) and asks the active
 * UWaveStrategy what to spawn, where and when.
 */
UCLASS()
class GADE7322POE_API UWaveDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickableInEditor() const override { return false; }

	bool IsDirectingWaves() const { return bDirecting; }

	/** Switches strategy. Takes effect from the next wave that is built. */
	UFUNCTION(BlueprintCallable, Category = "Waves")
	void SetStrategyClass(TSubclassOf<UWaveStrategy> NewClass);

	UFUNCTION(BlueprintCallable, Category = "Waves")
	void SkipIntermission();

	UFUNCTION(BlueprintPure, Category = "Waves")
	int32 GetCurrentWave() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category = "Waves")
	FString GetStrategyName() const;

	/** One-line wave status for UI, e.g. "Next wave in 6s" or "Enemies left: 4". */
	UFUNCTION(BlueprintPure, Category = "Waves")
	FString GetStatusText() const;

	UFUNCTION(BlueprintPure, Category = "Waves")
	int32 GetEnemiesRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Waves")
	FWaveContext GetLastContext() const { return LastContext; }

protected:
	enum class EPhase : uint8
	{
		Idle,
		Intermission,
		Spawning,
		Clearing
	};

	struct FTrackedEnemy
	{
		TWeakObjectPtr<AActor> Actor;
		int32 Wave = 0;
		int32 PathIndex = INDEX_NONE;
		float SpawnDistance = 0.f;
		FVector LastLocation = FVector::ZeroVector;
	};

	struct FTowerSample
	{
		float Time = 0.f;
		float HealthPercent = 1.f;
	};

	bool IsGameplayWorld() const;
	void ResolveClasses();
	AActor* FindFirstActor(UClass* Class) const;
	bool IsGamePlaying(AActor* GameManager) const;

	void EnsureStrategy(bool bApplyPending);
	FWaveLaneInfo* FindLane(int32 PathIndex);
	AActor* GetTower() const;
	void SuppressBlueprintSpawner(AActor* Spawner);
	void RefreshLanes(AActor* Spawner);
	void RefreshLaneCoverage();
	void SampleTower(float Now);
	void UpdateTrackedEnemies();
	FWaveContext BuildContext(int32 WaveNumber) const;

	void BeginIntermission(float Now);
	void BeginWave(float Now);
	void TickSpawning(float Now);
	void FinishWave(float Now);

	AActor* SpawnEnemy(AActor* Spawner, const FWaveSpawnEntry& Entry);
	AActor* CallSpawnEnemyOnPath(AActor* Spawner, int32 PathIndex) const;
	void ApplyModifiers(AActor* Enemy, const FWaveSpawnEntry& Entry) const;
	UClass* GetEnemyClass(EWaveEnemyType Type) const;

	float GetTowerHealthPercent() const;
	void DrawDebugHud() const;

	UPROPERTY()
	TObjectPtr<UWaveStrategy> Strategy;

	UPROPERTY()
	TSubclassOf<UWaveStrategy> PendingStrategyClass;

	UPROPERTY()
	TObjectPtr<UClass> GoblinClass;

	UPROPERTY()
	TObjectPtr<UClass> BerserkerClass;

	UPROPERTY()
	TObjectPtr<UClass> ShamanClass;

	UPROPERTY()
	TObjectPtr<UClass> ArcherClass;

	UPROPERTY()
	TObjectPtr<UClass> FrostArcherClass;

	UPROPERTY()
	TObjectPtr<UClass> GuardianClass;

	UPROPERTY()
	TObjectPtr<UClass> SpawnerClass;

	UPROPERTY()
	TObjectPtr<UClass> SpawnPointClass;

	UPROPERTY()
	TObjectPtr<UClass> TowerClass;

	UPROPERTY()
	TObjectPtr<UClass> GameManagerClass;

	TWeakObjectPtr<AActor> CachedSpawner;
	mutable TWeakObjectPtr<AActor> CachedTower;
	TWeakObjectPtr<AActor> CachedGameManager;

	TArray<FWaveLaneInfo> Lanes;
	TArray<FTrackedEnemy> TrackedEnemies;
	TArray<FTowerSample> TowerSamples;

	FWavePlan CurrentPlan;
	FWaveContext LastContext;
	FString LastPlanSummary;

	EPhase Phase = EPhase::Idle;
	int32 CurrentWave = 0;
	int32 SpawnCursor = 0;
	float PhaseEndTime = 0.f;
	float NextSpawnTime = 0.f;
	float WaveStartTime = 0.f;
	float LastSpawnTime = 0.f;
	float WaveStartTowerHealth = 1.f;

	int32 WaveSpawned = 0;
	int32 WaveKilled = 0;
	float WaveProgressSum = 0.f;

	bool bClassesResolved = false;
	bool bDirecting = false;
	bool bSpawnerStopped = false;
	float ClassResolveRetryAt = 0.f;
	float NextLaneRefreshTime = 0.f;
	float NextTowerSampleTime = 0.f;
};
