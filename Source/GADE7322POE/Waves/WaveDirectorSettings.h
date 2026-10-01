#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WaveDirectorSettings.generated.h"

class UWaveStrategy;


UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Procedural Waves"))
class GADE7322POE_API UWaveDirectorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWaveDirectorSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	
	UPROPERTY(Config, EditAnywhere, Category = "Waves")
	bool bEnableWaveDirector = true;

	
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (AllowAbstract = "false"))
	TSoftClassPtr<UWaveStrategy> StrategyClass;

	UPROPERTY(Config, EditAnywhere, Category = "Waves")
	bool bShowDebugHud = true;

	
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (ClampMin = "10.0"))
	float ClearTimeout = 150.f;

	
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (ClampMin = "1.0"))
	float StressWindow = 8.f;

	
	UPROPERTY(Config, EditAnywhere, Category = "Waves", meta = (ClampMin = "0.0"))
	float NearTowerRadius = 900.f;
};
