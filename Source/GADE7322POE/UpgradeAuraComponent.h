#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UpgradeAuraComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/** Soft translucent pink shell around an upgraded defender. */
UCLASS(ClassGroup = (Visual), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UUpgradeAuraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUpgradeAuraComponent();

	UFUNCTION(BlueprintCallable, Category = "Upgrade|Aura")
	void ShowAura();

	UFUNCTION(BlueprintCallable, Category = "Upgrade|Aura")
	void HideAura();

	UFUNCTION(BlueprintPure, Category = "Upgrade|Aura")
	bool IsAuraVisible() const { return bAuraVisible; }

	/** Adds the component (if needed) and shows the aura. */
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Aura", meta = (DefaultToSelf = "Defender"))
	static UUpgradeAuraComponent* ApplyTo(AActor* Defender);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Aura", meta = (ClampMin = "1.0"))
	float AuraScaleMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Aura")
	FVector AuraOffset = FVector(0.f, 0.f, 40.f);

protected:
	virtual void BeginPlay() override;

	void EnsureAuraMesh();

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> AuraMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> AuraMaterial;

	bool bAuraVisible = false;
};
