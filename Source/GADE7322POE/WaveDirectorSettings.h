#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WaveDirectorSettings.generated.h"

class UWaveStrategy;

/** Project Settings > Game > Procedural Waves. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Procedural Waves"))
class GADE7322POE_API UWaveDirectorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWaveDirectorSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** When off, the Part 1 Blueprint spawner runs as before. */
	UPROPERTY(Config, EditAnywhere, Category = "Waves")
	bool bEnableWaveDirector = true;

	/** Which procedural wave strategy runs. Can be switched in play with "td.WaveStrategy A|B". */
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (AllowAbstract = "false"))
	TSoftClassPtr<UWaveStrategy> StrategyClass;

	UPROPERTY(Config, EditAnywhere, Category = "Waves")
	bool bShowDebugHud = true;

	/** A wave ends anyway if its last enemies are still alive this long after the final spawn. */
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (ClampMin = "10.0"))
	float ClearTimeout = 150.f;

	/** Seconds of tower damage history used for the live stress measurement. */
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (ClampMin = "1.0"))
	float StressWindow = 8.f;

	/** Enemies closer than this to the tower count as "near the tower" for stress. */
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (ClampMin = "0.0"))
	float NearTowerRadius = 900.f;
};
