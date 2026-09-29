#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyStatusComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

USTRUCT()
struct FStatusSavedMeshMaterials
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TintMaterial = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySlowChanged, bool, bIsSlowed);

/**
 * Temporary status effects for path-following enemies (BP_GoblinEnemy and children).
 * Never owns movement: it only scales the enemy's existing MoveSpeed variable, which
 * the Blueprint MoveAlongPath already reads every tick.
 *
 * MoveSpeed = BaseMoveSpeed * SlowMultiplier * SpeedBoostMultiplier
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UEnemyStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyStatusComponent();

	static UEnemyStatusComponent* FindOrAddTo(AActor* Enemy);

	/** Slows the owner by SlowPercent (0.4 = 40% slower) for Duration seconds. Re-hits refresh, never stack. */
	UFUNCTION(BlueprintCallable, Category = "Status|Slow")
	void ApplySlow(float SlowPercent, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Status|Slow")
	void ClearSlow();

	UFUNCTION(BlueprintPure, Category = "Status|Slow")
	bool IsSlowed() const { return SlowTimeRemaining > 0.f; }

	/** Extra speed multiplier used by abilities such as Berserker rage (1 = none). */
	UFUNCTION(BlueprintCallable, Category = "Status|Speed")
	void SetSpeedBoost(float Multiplier);

	/** Permanent body colour for unit variants (e.g. Berserker). Slowed tint is blended on top. */
	UFUNCTION(BlueprintCallable, Category = "Status|Visual")
	void SetBodyTint(bool bEnable, FLinearColor Color);

	UFUNCTION(BlueprintPure, Category = "Status|Speed")
	float GetBaseMoveSpeed() const { return BaseMoveSpeed; }

	UPROPERTY(BlueprintAssignable, Category = "Status|Slow")
	FOnEnemySlowChanged OnSlowChanged;

	/** Blueprint float/double variable on the owner that drives path movement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	FName SpeedPropertyName = TEXT("MoveSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Visual")
	FLinearColor SlowedTint = FLinearColor(0.35f, 0.8f, 1.f);

	/** How strongly the slowed tint replaces the body colour when the enemy has one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowedTintStrength = 0.6f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float BaseMoveSpeed = -1.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float SlowMultiplier = 1.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float SlowTimeRemaining = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float SpeedBoostMultiplier = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void CaptureBaseSpeed();
	void ApplySpeed();
	void RefreshVisual();
	void RestoreMaterials();

	float LastWrittenSpeed = -1.f;
	bool bHasBodyTint = false;
	FLinearColor BodyTint = FLinearColor::White;

	bool bVisualApplied = false;
	FLinearColor LastAppliedColor = FLinearColor::Transparent;

	UPROPERTY()
	TArray<FStatusSavedMeshMaterials> SavedMaterials;
};
