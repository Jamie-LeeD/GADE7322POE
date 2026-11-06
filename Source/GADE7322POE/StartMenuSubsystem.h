#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StartMenuSubsystem.generated.h"

class UStartMenuWidget;
class UWaveStrategy;

/** Shows the start / strategy picker and holds the match until the player chooses a wave mode. */
UCLASS()
class GADE7322POE_API UStartMenuSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }

	UFUNCTION(BlueprintPure, Category = "Start Menu")
	bool HasStartedMatch() const { return bMatchStarted; }

protected:
	bool IsGameplayWorld() const;
	void ResolveClasses();
	AActor* FindGameManager() const;
	void SetGameStatePlaying(bool bPlaying);
	void ShowMenu();
	void ApplyInputForMenu();
	void ApplyInputForGame();

	UFUNCTION()
	void HandleStrategyChosen(bool bBudgetStrategy);

	UPROPERTY()
	TObjectPtr<UStartMenuWidget> MenuWidget;

	UPROPERTY()
	TObjectPtr<UClass> GameManagerClass;

	bool bClassesResolved = false;
	bool bMenuShown = false;
	bool bMatchStarted = false;
	float ClassResolveRetryAt = 0.f;
};
