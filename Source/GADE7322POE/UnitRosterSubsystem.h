#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnitRosterSubsystem.generated.h"

/**
 * Plugs the Part 2 unit variants into the existing Blueprint systems without editing them:
 *  - BP_EnemySpawner: after each spawn, its GoblinClass is re-rolled between Goblin and Goblin Berserker.
 *  - BP_DefenderPlacementManager: keys 1/2 swap its ArcherClass between Royal Archer and Frost Archer;
 *    TryPlaceArcher still does the slot checks, charges its base cost and spawns. The Frost Archer's
 *    extra cost is charged here (or the placement is refunded if the player can't afford it).
 *  - Guarantees the variant C++ components exist on spawned variants.
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
	void TickPlacementInput();
	void TickFrostPlacementCharges();
	void EnsureVariantComponents();

	void SetFrostPlacementMode(bool bFrost);
	int32 GetFrostArcherCost() const;
	bool CallGoldFunction(AActor* GameManager, FName FunctionName, int32 Amount) const;
	void ShowMessage(int32 Key, const FString& Text, const FColor& Color, float Seconds = 3.f) const;

	UPROPERTY()
	TObjectPtr<UClass> GoblinClass;

	UPROPERTY()
	TObjectPtr<UClass> BerserkerClass;

	UPROPERTY()
	TObjectPtr<UClass> FrostArcherClass;

	UPROPERTY()
	TObjectPtr<UClass> SpawnerClass;

	UPROPERTY()
	TObjectPtr<UClass> PlacementManagerClass;

	UPROPERTY()
	TObjectPtr<UClass> GameManagerClass;

	UPROPERTY()
	TObjectPtr<UClass> OriginalArcherClass;

	TSet<TWeakObjectPtr<AActor>> KnownEnemies;
	TSet<TWeakObjectPtr<AActor>> KnownFrostArchers;

	FRandomStream Random;
	bool bClassesResolved = false;
	float ClassResolveRetryAt = 0.f;
	float PlayStartTime = -1.f;
	float NextComponentCheckTime = 0.f;
	bool bFrostMode = false;
	bool bKeyOneDown = false;
	bool bKeyTwoDown = false;
	bool bFrostArchersSeeded = false;
	bool bSpawnRollPending = true;
};
