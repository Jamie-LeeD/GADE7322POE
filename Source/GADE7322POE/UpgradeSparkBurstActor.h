#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UpgradeSparkBurstActor.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** Short-lived custom particle burst (upgrade sparks) when Cascade/Niagara assets are empty. */
UCLASS()
class GADE7322POE_API AUpgradeSparkBurstActor : public AActor
{
	GENERATED_BODY()

public:
	AUpgradeSparkBurstActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void PlayBurst(int32 SparkCount = 28);

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void SetSparkColor(FLinearColor InColor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FLinearColor SparkColor = FLinearColor(1.f, 0.75f, 0.2f);

protected:
	struct FSpark
	{
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
		FVector Velocity = FVector::ZeroVector;
		float Life = 0.f;
		float MaxLife = 0.4f;
		float StartScale = 8.f;
	};

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SparkMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SparkMesh;

	TArray<FSpark> Sparks;
	float Age = 0.f;
	bool bPlaying = false;
};
