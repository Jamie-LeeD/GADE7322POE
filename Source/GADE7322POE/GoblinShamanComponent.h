#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoblinShamanComponent.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShamanSupportPulse, int32, AlliesBuffed);

/**
 * Goblin Shaman behaviour layered on top of BP_GoblinEnemy.
 * Pathing, health and death stay in the existing goblin systems. The combat director fires a magic
 * bolt (existing projectile system) instead of a melee hit, and this component periodically buffs
 * nearby goblins through their UEnemyStatusComponent (temporary, refreshes rather than stacks).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UGoblinShamanComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGoblinShamanComponent();

	/** Buffs every living goblin (any BP_GoblinEnemy child) in SupportRadius, except the Shaman itself. */
	UFUNCTION(BlueprintCallable, Category = "Shaman|Support")
	int32 CastSupportPulse();

	/** Recolours a spawned bolt so Shaman attacks are readable. */
	void TintProjectile(AActor* Projectile) const;

	UPROPERTY(BlueprintAssignable, Category = "Shaman|Support")
	FOnShamanSupportPulse OnSupportPulse;

	/** When true the combat director fires a magic bolt at targets in AttackRange instead of a melee hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Attack")
	bool bUseMagicBolt = true;

	/** Written into the inherited BP_HealthComponent at BeginPlay (0 = keep the goblin value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Stats", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.0"))
	float SupportRadius = 600.f;

	/** Seconds between support pulses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.1"))
	float SupportInterval = 3.f;

	/** How long each pulse's buff lasts on an ally (refreshed by later pulses). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.1"))
	float BuffDuration = 4.f;

	/** 0.2 = +20% movement speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.0"))
	float BuffSpeedBonus = 0.2f;

	/** 0.15 = +15% attack damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.0"))
	float BuffDamageBonus = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Visual")
	FLinearColor BodyColor = FLinearColor(0.3f, 0.08f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Visual")
	FLinearColor StaffColor = FLinearColor(0.35f, 0.2f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Visual")
	FLinearColor MagicColor = FLinearColor(0.35f, 1.f, 0.4f);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool IsAllyAlive(const AActor* Ally) const;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaffMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> AuraMesh;

	UPROPERTY()
	TObjectPtr<UClass> GoblinClass;

	FVector OrbRestLocation = FVector::ZeroVector;
	FVector AuraFullScale = FVector::OneVector;
	float PulseCooldown = 1.f;
	float AuraTimeRemaining = 0.f;
	float Age = 0.f;
};
