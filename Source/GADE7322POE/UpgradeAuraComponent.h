#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UpgradeAuraComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Soft translucent aura shell around an upgraded defender (blue Lv1, pink Lv2+). */
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

	/** Sets visual tier: 1 = blue, 2+ = pink. */
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Aura")
	void SetUpgradeLevel(int32 InUpgradeLevel);

	UFUNCTION(BlueprintPure, Category = "Upgrade|Aura")
	int32 GetUpgradeLevel() const { return UpgradeLevel; }

	/** Adds the component (if needed) and shows the aura for the given upgrade level. */
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Aura", meta = (DefaultToSelf = "Defender"))
	static UUpgradeAuraComponent* ApplyTo(AActor* Defender, int32 InUpgradeLevel = 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Aura", meta = (ClampMin = "1.0"))
	float AuraScaleMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Aura")
	FVector AuraOffset = FVector(0.f, 0.f, 40.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Aura")
	FLinearColor Level1Color = FLinearColor(0.2f, 0.55f, 1.0f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Aura")
	FLinearColor Level2Color = FLinearColor(1.0f, 0.25f, 0.75f, 1.f);

protected:
	virtual void BeginPlay() override;

	void EnsureAuraMesh();
	void ApplyAuraColor();
	FLinearColor GetColorForLevel(int32 Level) const;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> AuraMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> AuraMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> AuraMID;

	int32 UpgradeLevel = 1;
	bool bAuraVisible = false;
	bool bAuraScaleLocked = false;
	float LockedAuraScale = 1.f;
};
