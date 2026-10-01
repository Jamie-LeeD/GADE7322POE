#pragma once

#include "CoreMinimal.h"
#include "Waves/WaveStrategy.h"
#include "BudgetWaveStrategy.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "Threat Budget Strategy (A)"))
class GADE7322POE_API UBudgetWaveStrategy : public UWaveStrategy
{
	GENERATED_BODY()

public:
	virtual FString GetStrategyName() const override { return TEXT("Threat Budget (A)"); }
	virtual void ResetStrategy(int32 Seed) override;
	virtual FWavePlan BuildWave(const FWaveContext& Context) override;
	virtual void OnWaveFinished(const FWaveReport& Report) override;
	virtual float GetIntermissionDuration(int32 UpcomingWave) const override;
	virtual FString GetDebugStatus() const override;

protected:
	float GetEnemyCost(EWaveEnemyType Type) const;
	bool IsMilestoneWave(int32 Wave) const;
	bool IsBreatherWave(int32 Wave) const;
	float ComputeBudget(int32 Wave) const;
	int32 PickLane(const FWaveContext& Context);

	// Budget curve: (BaseBudget + BudgetPerWave * (w - 1)) * BudgetGrowth^(w - 1) * SkillRating
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Budget", meta = (ClampMin = "1.0"))
	float BaseBudget = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Budget", meta = (ClampMin = "0.0"))
	float BudgetPerWave = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Budget", meta = (ClampMin = "1.0"))
	float BudgetGrowth = 1.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Budget", meta = (ClampMin = "1"))
	int32 MilestoneEvery = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Budget", meta = (ClampMin = "1.0"))
	float MilestoneBudgetScale = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Budget", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float BreatherBudgetScale = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Budget", meta = (ClampMin = "1"))
	int32 MaxEnemiesPerWave = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "0.1"))
	float GoblinCost = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "0.1"))
	float BerserkerCost = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "0.1"))
	float ShamanCost = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "1"))
	int32 BerserkerUnlockWave = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "1"))
	int32 ShamanUnlockWave = 5;

	
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "1"))
	int32 EnemiesPerShaman = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "0.0"))
	float BerserkerWeight = 0.6f;

	
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "0.0"))
	float BerserkerWeightPerWave = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Enemies", meta = (ClampMin = "0.0"))
	float ShamanWeight = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Skill", meta = (ClampMin = "0.1"))
	float MinSkillRating = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Skill", meta = (ClampMin = "0.1"))
	float MaxSkillRating = 1.6f;

	
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Skill", meta = (ClampMin = "0.0"))
	float SkillLearnRate = 0.12f;

	
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Skill", meta = (ClampMin = "0.01"))
	float TargetTowerDamage = 0.08f;

	
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Skill", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float TargetProgress = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Lanes", meta = (ClampMin = "0.0"))
	float MinLaneWeight = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Lanes", meta = (ClampMin = "1"))
	int32 MinSquadSize = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Lanes", meta = (ClampMin = "1"))
	int32 MaxSquadSize = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.1"))
	float BaseSpawnInterval = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float SpawnIntervalDecay = 0.95f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.1"))
	float MinSpawnInterval = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.0"))
	float SquadGap = 2.5f;

	float SkillRating = 1.f;
	float LastPerformance = 0.f;
	float LastBudget = 0.f;
};
