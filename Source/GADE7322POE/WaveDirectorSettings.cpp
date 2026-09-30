#include "WaveDirectorSettings.h"

#include "BudgetWaveStrategy.h"

UWaveDirectorSettings::UWaveDirectorSettings()
{
	StrategyClass = UBudgetWaveStrategy::StaticClass();
}
