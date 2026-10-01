#include "Waves/WaveDirectorSettings.h"

#include "Waves/BudgetWaveStrategy.h"

UWaveDirectorSettings::UWaveDirectorSettings()
{
	StrategyClass = UBudgetWaveStrategy::StaticClass();
}
