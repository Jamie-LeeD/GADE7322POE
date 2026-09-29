#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrostAttackComponent.generated.h"

class UMaterialInstanceDynamic;

/**
 * Frost Archer on-hit effect layered on top of BP_RoyalArcher.
 * Targeting, firing and projectile travel stay in the existing archer/combat systems;
 * the combat director calls ApplyFrostOnHit when this unit's projectile lands.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UFrostAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFrostAttackComponent();

	/** Applies (or refreshes) the slow on an enemy through its UEnemyStatusComponent. */
	UFUNCTION(BlueprintCallable, Category = "Frost")
	void ApplyFrostOnHit(AActor* Target) const;

	/** Recolours a spawned projectile visual so frost shots are readable. */
	void TintProjectile(AActor* Projectile) const;

	/** Written into the inherited BP_HealthComponent at BeginPlay (0 = keep the archer value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frost|Stats", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 120.f;

	/** 0.4 = target moves 40% slower. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frost|Slow", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float SlowPercent = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frost|Slow", meta = (ClampMin = "0.1"))
	float SlowDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frost|Visual")
	FLinearColor BodyColor = FLinearColor(0.25f, 0.65f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frost|Visual")
	FLinearColor ProjectileColor = FLinearColor(0.6f, 0.95f, 1.f);

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
};
