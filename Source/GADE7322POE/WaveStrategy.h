#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WaveTypes.h"
#include "WaveStrategy.generated.h"

/**
 * Decides what each wave contains. The wave director owns the wave loop, spawning and player
 * measurement; a strategy only turns a FWaveContext into a FWavePlan and learns from FWaveReports.
 */
UCLASS(Abstract, Blueprintable)
class GADE7322POE_API UWaveStrategy : public UObject
{
	GENERATED_BODY()

public:
	virtual FString GetStrategyName() const { return GetClass()->GetName(); }

	/** Called once when the director starts running waves with this strategy. */
	virtual void ResetStrategy(int32 Seed);

	virtual FWavePlan BuildWave(const FWaveContext& Context) PURE_VIRTUAL(UWaveStrategy::BuildWave, return FWavePlan(););

	virtual void OnWaveFinished(const FWaveReport& Report) {}

	/** Multiplier applied to every spawn delay while a wave is running. Above 1 slows spawning down. */
	virtual float GetPacingScale(const FWaveContext& Context) const { return 1.f; }

	virtual float GetIntermissionDuration(int32 UpcomingWave) const;

	/** Extra line for the debug HUD (skill rating, stress, etc.). */
	virtual FString GetDebugStatus() const { return FString(); }

	static const TCHAR* GetEnemyTypeName(EWaveEnemyType Type);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.0"))
	float FirstIntermission = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.0"))
	float Intermission = 8.f;

	/** Picks an index from Weights proportionally to its weight (zero weights are never picked). */
	int32 PickWeighted(const TArray<float>& Weights);

	FRandomStream Random;
};
