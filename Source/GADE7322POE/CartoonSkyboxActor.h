#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CartoonSkyboxActor.generated.h"

class UStaticMeshComponent;

/**
 * Equirectangular sky sphere that stays centered on the camera so the
 * painted panorama (including lower sky-ground) always wraps 360°.
 */
UCLASS()
class GADE7322POE_API ACartoonSkyboxActor : public AActor
{
	GENERATED_BODY()

public:
	ACartoonSkyboxActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skybox")
	TObjectPtr<UStaticMeshComponent> SkyMesh;

	/** Uniform scale of the Engine sphere mesh. Preview_Sky0002_Day uses 100. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skybox")
	float SphereScale = 100.f;

private:
	/** Real-time SkyLight + a moving sky sphere makes ground tiles flash. */
	void StabilizeSkyLights() const;
};
