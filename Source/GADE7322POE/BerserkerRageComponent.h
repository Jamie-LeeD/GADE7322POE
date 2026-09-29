#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BerserkerRageComponent.generated.h"

class UEnemyStatusComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBerserkerRageChanged, bool, bIsRaging);

/**
 * Goblin Berserker behaviour layered on top of BP_GoblinEnemy.
 * Movement, pathing, melee and death stay in the existing goblin/combat systems;
 * this component only tints the body and triggers a temporary Rage at low health.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UBerserkerRageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBerserkerRageComponent();

	UFUNCTION(BlueprintPure, Category = "Berserker|Rage")
	bool IsRaging() const { return bIsRaging; }

	UFUNCTION(BlueprintCallable, Category = "Berserker|Rage")
	void StartRage();

	UFUNCTION(BlueprintCallable, Category = "Berserker|Rage")
	void EndRage();

	UPROPERTY(BlueprintAssignable, Category = "Berserker|Rage")
	FOnBerserkerRageChanged OnRageChanged;

	/** Written into the inherited BP_HealthComponent at BeginPlay (0 = keep the goblin value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Stats", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 60.f;

	/** Rage triggers when CurrentHealth / MaxHealth is at or below this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Rage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RageHealthThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Rage", meta = (ClampMin = "0.1"))
	float RageDuration = 4.f;

	/** Time after Rage ends before it can trigger again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Rage", meta = (ClampMin = "0.0"))
	float RageCooldown = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Rage", meta = (ClampMin = "1.0"))
	float RageSpeedMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Rage", meta = (ClampMin = "1.0"))
	float RageDamageMultiplier = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Visual")
	FLinearColor BodyColor = FLinearColor(0.45f, 0.04f, 0.02f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Visual")
	FLinearColor RageColor = FLinearColor(1.f, 0.35f, 0.f);

	/** Mesh scale relative to the goblin (stockier silhouette). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Visual")
	FVector BodyScale = FVector(1.2f, 1.2f, 0.9f);

	/** Extra scale pulse amplitude while raging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Berserker|Visual", meta = (ClampMin = "0.0"))
	float RagePulseScale = 0.15f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Berserker|Debug")
	bool bIsRaging = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Berserker|Debug")
	float RageTimeRemaining = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Berserker|Debug")
	float CooldownRemaining = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetHealthPercent() const;
	void ApplyMeshScale(float PulseAlpha);

	float BaseAttackDamage = -1.f;
	float RageAge = 0.f;

	UPROPERTY()
	TObjectPtr<UEnemyStatusComponent> Status;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BodyMeshes;

	TArray<FVector> OriginalMeshScales;
};
