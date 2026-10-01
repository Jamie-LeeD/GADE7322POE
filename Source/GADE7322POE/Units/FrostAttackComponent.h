#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrostAttackComponent.generated.h"

class UMaterialInstanceDynamic;


UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UFrostAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFrostAttackComponent();

	
	UFUNCTION(BlueprintCallable, Category = "Frost")
	void ApplyFrostOnHit(AActor* Target) const;

	
	void TintProjectile(AActor* Projectile) const;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frost|Stats", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 120.f;

	
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
