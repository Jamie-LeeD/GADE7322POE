#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoblinShamanComponent.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShamanSupportPulse, int32, AlliesBuffed);


UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UGoblinShamanComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGoblinShamanComponent();

	
	UFUNCTION(BlueprintCallable, Category = "Shaman|Support")
	int32 CastSupportPulse();

	
	void TintProjectile(AActor* Projectile) const;

	UPROPERTY(BlueprintAssignable, Category = "Shaman|Support")
	FOnShamanSupportPulse OnSupportPulse;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Attack")
	bool bUseMagicBolt = true;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Stats", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.0"))
	float SupportRadius = 600.f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.1"))
	float SupportInterval = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.1"))
	float BuffDuration = 4.f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shaman|Support", meta = (ClampMin = "0.0"))
	float BuffSpeedBonus = 0.2f;

	
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
