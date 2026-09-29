#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnitRosterSubsystem.generated.h"

/**
 * Plugs the Part 2 unit variants into the existing Blueprint systems without editing them:
 *  - BP_EnemySpawner: after each spawn, its GoblinClass is re-rolled between Goblin, Goblin Berserker
 *    and Goblin Shaman (stand-in until the procedural wave system picks classes itself).
 *  - Guarantees the variant C++ components exist on spawned variants.
 * Defender selection/placement lives in ADefenderPlacementManagerBase (BP_DefenderPlacementManager).
 */
UCLASS()
class GADE7322POE_API UUnitRosterSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickableInEditor() const override { return false; }

protected:
	bool IsGameplayWorld() const;
	void ResolveClasses();
	AActor* FindFirstActor(UClass* Class) const;
	bool IsGamePlaying(AActor* GameManager) const;

	void TickSpawnMix(float Now);
	void EnsureVariantComponents();

	UPROPERTY()
	TObjectPtr<UClass> GoblinClass;

	UPROPERTY()
	TObjectPtr<UClass> BerserkerClass;

	UPROPERTY()
	TObjectPtr<UClass> FrostArcherClass;

	UPROPERTY()
	TObjectPtr<UClass> ShamanClass;

	UPROPERTY()
	TObjectPtr<UClass> GuardianClass;

	UPROPERTY()
	TObjectPtr<UClass> SpawnerClass;

	UPROPERTY()
	TObjectPtr<UClass> GameManagerClass;

	TSet<TWeakObjectPtr<AActor>> KnownEnemies;

	FRandomStream Random;
	bool bClassesResolved = false;
	float ClassResolveRetryAt = 0.f;
	float PlayStartTime = -1.f;
	float NextComponentCheckTime = 0.f;
	bool bSpawnRollPending = true;
};
