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

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UEnemyStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyStatusComponent();

	static UEnemyStatusComponent* FindOrAddTo(AActor* Enemy);

	
	UFUNCTION(BlueprintCallable, Category = "Status|Slow")
	void ApplySlow(float SlowPercent, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Status|Slow")
	void ClearSlow();

	UFUNCTION(BlueprintPure, Category = "Status|Slow")
	bool IsSlowed() const { return SlowTimeRemaining > 0.f; }

	
	UFUNCTION(BlueprintCallable, Category = "Status|Speed")
	void SetSpeedBoost(float Multiplier);

	
	UFUNCTION(BlueprintCallable, Category = "Status|Damage")
	void SetDamageBoost(float Multiplier);


	UFUNCTION(BlueprintCallable, Category = "Status|Support")
	void ApplySupportBuff(float SpeedBonus, float DamageBonus, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Status|Support")
	void ClearSupportBuff();

	UFUNCTION(BlueprintPure, Category = "Status|Support")
	bool HasSupportBuff() const { return SupportTimeRemaining > 0.f; }

	
	UFUNCTION(BlueprintPure, Category = "Status|Damage")
	float GetDamageMultiplier() const { return DamageBoostMultiplier * SupportDamageMultiplier; }

	
	UFUNCTION(BlueprintCallable, Category = "Status|Visual")
	void SetBodyTint(bool bEnable, FLinearColor Color);

	UFUNCTION(BlueprintPure, Category = "Status|Speed")
	float GetBaseMoveSpeed() const { return BaseMoveSpeed; }

	UPROPERTY(BlueprintAssignable, Category = "Status|Slow")
	FOnEnemySlowChanged OnSlowChanged;

	UPROPERTY(BlueprintAssignable, Category = "Status|Support")
	FOnEnemySlowChanged OnSupportBuffChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Visual")
	FLinearColor SupportBuffTint = FLinearColor(0.3f, 1.f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SupportBuffTintStrength = 0.45f;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	FName SpeedPropertyName = TEXT("MoveSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Visual")
	FLinearColor SlowedTint = FLinearColor(0.35f, 0.8f, 1.f);

	
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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float DamageBoostMultiplier = 1.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float SupportSpeedMultiplier = 1.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float SupportDamageMultiplier = 1.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Debug")
	float SupportTimeRemaining = 0.f;

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
