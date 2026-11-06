#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectileTrailComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class UParticleSystem;
class UParticleSystemComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Particle trail attached to combat projectiles (Cascade/Niagara + mesh spark fallback). */
UCLASS(ClassGroup = (VFX), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UProjectileTrailComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UProjectileTrailComponent();

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void Configure(FLinearColor InTrailColor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FLinearColor TrailColor = FLinearColor(1.f, 0.75f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX", meta = (ClampMin = "0.01"))
	float SparkInterval = 0.028f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX", meta = (ClampMin = "1"))
	int32 SparksPerEmit = 2;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void EnsureAssetFx();
	void EmitMeshSparks(int32 Count);
	void TickMeshSparks(float DeltaTime);

	struct FTrailSpark
	{
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
		FVector Velocity = FVector::ZeroVector;
		float Life = 0.f;
		float MaxLife = 0.25f;
		float StartScale = 4.f;
	};

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SparkMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SparkMID;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SparkMesh;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> TrailNiagara;

	UPROPERTY()
	TObjectPtr<UParticleSystem> TrailCascade;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> CascadeComp;

	TArray<FTrailSpark> Sparks;
	float EmitCooldown = 0.f;
	bool bConfigured = false;
};
