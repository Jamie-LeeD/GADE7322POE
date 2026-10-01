#pragma once

#include "CoreMinimal.h"
#include "WaveStrategy.h"
#include "CounterWaveStrategy.generated.h"

/**
 * Strategy B - Counter-Play Director.
 * Wave size follows a repeating intensity curve (build-up, build-up, peak, relief) on a rising
 * baseline. The enemy mix counters the player's defender composition, squads target the least
 * defended lane, hoarding gold summons extra enemies, and spawn pacing reacts live to tower stress.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Counter-Play Strategy (B)"))
class GADE7322POE_API UCounterWaveStrategy : public UWaveStrategy
{
	GENERATED_BODY()

public:
	virtual FString GetStrategyName() const override { return TEXT("Counter-Play (B)"); }
	virtual void ResetStrategy(int32 Seed) override;
	virtual FWavePlan BuildWave(const FWaveContext& Context) override;
	virtual void OnWaveFinished(const FWaveReport& Report) override;
	virtual float GetPacingScale(const FWaveContext& Context) const override;
	virtual FString GetDebugStatus() const override;

protected:
	float GetIntensity(int32 Wave) const;
	bool IsPeakWave(int32 Wave) const;
	float ComputeStress(const FWaveContext& Context) const;
	void ComputeCounterShares(const FWaveContext& Context, float& OutBerserker, float& OutShaman) const;
	TArray<int32> RankLanesByWeakness(const FWaveContext& Context) const;

	/** Intensity multiplier for each wave of a repeating cycle. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Intensity")
	TArray<float> IntensityPattern = { 0.8f, 1.f, 1.4f, 0.6f };

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Intensity", meta = (ClampMin = "1"))
	int32 BaseEnemies = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Intensity", meta = (ClampMin = "0.0"))
	float EnemiesPerWave = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Intensity", meta = (ClampMin = "1"))
	int32 MaxEnemiesPerWave = 28;

	/** From this wave on enemies also gain HealthPerWave extra health per wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Intensity", meta = (ClampMin = "1"))
	int32 HealthScalingStartWave = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Intensity", meta = (ClampMin = "0.0"))
	float HealthPerWave = 0.06f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "1"))
	int32 BerserkerUnlockWave = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "1"))
	int32 ShamanUnlockWave = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseBerserkerShare = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseShamanShare = 0.1f;

	/** Berserker share added when every defender is a regular (non-frost) archer. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BerserkerVsArchers = 0.35f;

	/** Shaman share added when every defender is a Royal Guardian. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShamanVsGuardians = 0.3f;

	/** Shaman share added when every defender is a Frost Archer (the speed buff cancels the slow). */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShamanVsFrost = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Counter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinGoblinShare = 0.3f;

	/** Greed is not punished before this wave, so the starting gold has time to be spent. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Greed", meta = (ClampMin = "1"))
	int32 GreedStartWave = 3;

	/** Unspent gold above this at wave start means the player is hoarding. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Greed", meta = (ClampMin = "0.0"))
	float GreedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Greed", meta = (ClampMin = "1.0"))
	float GoldPerGreedEnemy = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Greed", meta = (ClampMin = "0"))
	int32 MaxGreedEnemies = 4;

	/** Share of squads sent to the weakest lane on normal waves. Peak waves split across every lane. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Lanes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeakestLaneShare = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Lanes", meta = (ClampMin = "1"))
	int32 MinSquadSize = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Lanes", meta = (ClampMin = "1"))
	int32 MaxSquadSize = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.1"))
	float InSquadInterval = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.1"))
	float BaseSquadGap = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.1"))
	float MinSquadGap = 1.5f;

	/** Tower damage in the stress window (fraction of max HP) that counts as full stress. */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Stress", meta = (ClampMin = "0.01"))
	float StressDamageReference = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Stress", meta = (ClampMin = "0.0"))
	float StressPerEnemyNearTower = 0.15f;

	/** Spawn delays are multiplied by this at full stress (mercy) ... */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Stress", meta = (ClampMin = "1.0"))
	float HighStressPacing = 1.8f;

	/** ... and by this when the player is completely relaxed (pressure). */
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Stress", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float LowStressPacing = 0.7f;

	/** Momentum from the last wave: scales the next wave's enemy count. */
	float Momentum = 1.f;
	float LastBerserkerShare = 0.f;
	float LastShamanShare = 0.f;
	int32 LastGreedEnemies = 0;
	mutable float LastStress = 0.f;
};
