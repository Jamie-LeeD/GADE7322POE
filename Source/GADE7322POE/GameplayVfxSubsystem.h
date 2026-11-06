#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayVfxSubsystem.generated.h"

class APostProcessVolume;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UParticleSystem;

/**
 * Part 3 VFX:
 * - Custom Niagara burst on defender type upgrades
 * - Custom screen-space post-process vignette when the princess tower is threatened
 */
UCLASS()
class GADE7322POE_API UGameplayVfxSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickableInEditor() const override { return false; }

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void PlayUpgradeBurstAt(FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void PlayUpgradeBurstForActor(AActor* Actor);

	/** Attach a particle trail to a combat projectile (gold / frost / shaman tint). */
	UFUNCTION(BlueprintCallable, Category = "VFX")
	void AttachProjectileTrail(AActor* Projectile, FLinearColor TrailColor);

	/** Small impact spark burst when a projectile hits. */
	UFUNCTION(BlueprintCallable, Category = "VFX")
	void PlayProjectileImpactAt(FVector WorldLocation, FLinearColor ImpactColor);

protected:
	bool IsGameplayWorld() const;
	void EnsureAssets();
	void EnsurePostProcessVolume();
	void UpdateTowerThreat(float DeltaTime);
	float GetTowerHealthPercent() const;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> UpgradeBurstSystem;

	UPROPERTY()
	TObjectPtr<UParticleSystem> UpgradeBurstCascade;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ThreatPostProcessMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ThreatMID;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> ThreatVolume;

	UPROPERTY()
	TObjectPtr<UClass> TowerClass;

	float ThreatIntensity = 0.f;
	bool bAssetsResolved = false;
	float AssetRetryAt = 0.f;
};
