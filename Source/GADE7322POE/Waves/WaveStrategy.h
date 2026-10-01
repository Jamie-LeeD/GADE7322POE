#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Waves/WaveTypes.h"
#include "WaveStrategy.generated.h"


UCLASS(Abstract, Blueprintable)
class GADE7322POE_API UWaveStrategy : public UObject
{
	GENERATED_BODY()

public:
	virtual FString GetStrategyName() const { return GetClass()->GetName(); }

	virtual void ResetStrategy(int32 Seed);

	virtual FWavePlan BuildWave(const FWaveContext& Context) PURE_VIRTUAL(UWaveStrategy::BuildWave, return FWavePlan(););

	virtual void OnWaveFinished(const FWaveReport& Report) {}


	virtual float GetPacingScale(const FWaveContext& Context) const { return 1.f; }

	virtual float GetIntermissionDuration(int32 UpcomingWave) const;

	virtual FString GetDebugStatus() const { return FString(); }

	static const TCHAR* GetEnemyTypeName(EWaveEnemyType Type);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.0"))
	float FirstIntermission = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|Pacing", meta = (ClampMin = "0.0"))
	float Intermission = 8.f;


	int32 PickWeighted(const TArray<float>& Weights);

	FRandomStream Random;
};
