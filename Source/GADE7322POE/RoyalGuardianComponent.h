#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RoyalGuardianComponent.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldGuardChanged, bool, bIsActive);


UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API URoyalGuardianComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URoyalGuardianComponent();

	
	UFUNCTION(BlueprintPure, Category = "Guardian|Shield Guard")
	float GetIncomingDamageMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Guardian|Shield Guard")
	bool IsShieldGuardActive() const { return bShieldGuardActive; }

	UFUNCTION(BlueprintCallable, Category = "Guardian|Shield Guard")
	void ActivateShieldGuard();

	UFUNCTION(BlueprintCallable, Category = "Guardian|Shield Guard")
	void EndShieldGuard();

	
	void NotifyMeleeStrike(AActor* Target);

	UPROPERTY(BlueprintAssignable, Category = "Guardian|Shield Guard")
	FOnShieldGuardChanged OnShieldGuardChanged;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Attack")
	bool bUseMeleeAttack = true;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Stats", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 400.f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Shield Guard", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float ShieldDamageReduction = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Shield Guard", meta = (ClampMin = "0.1"))
	float ShieldDuration = 3.f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Shield Guard", meta = (ClampMin = "0.0"))
	float ShieldCooldown = 12.f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Shield Guard", meta = (ClampMin = "0.0"))
	float ShieldTriggerRange = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Visual")
	FLinearColor BodyColor = FLinearColor(0.32f, 0.08f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Visual")
	FLinearColor ShieldGuardBodyColor = FLinearColor(0.55f, 0.6f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Visual")
	FLinearColor ShieldColor = FLinearColor(0.85f, 0.65f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Visual")
	FLinearColor ShieldActiveColor = FLinearColor(0.7f, 0.95f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Visual")
	FLinearColor SwordColor = FLinearColor(0.8f, 0.8f, 0.85f);

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardian|Visual")
	FVector BodyScale = FVector(1.45f, 1.45f, 1.15f);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guardian|Debug")
	bool bShieldGuardActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guardian|Debug")
	float ShieldTimeRemaining = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guardian|Debug")
	float ShieldCooldownRemaining = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool IsEnemyNearby() const;
	void ApplyBodyColor(const FLinearColor& Color);
	void UpdateShieldPose();

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BodyMeshes;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> SwordMesh;

	UPROPERTY()
	TObjectPtr<UClass> EnemyClass;

	FVector ShieldRestLocation = FVector::ZeroVector;
	FVector ShieldRestScale = FVector::OneVector;
	FRotator SwordRestRotation = FRotator::ZeroRotator;
	float SwingTimeRemaining = 0.f;
	float EnemyCheckCooldown = 0.f;
};
