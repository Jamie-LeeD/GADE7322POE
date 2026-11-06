#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TowerFabMeshComponent.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ETowerFabMeshKind : uint8
{
	Auto UMETA(DisplayName = "Auto (from owner class)"),
	Princess UMETA(DisplayName = "Princess Tower"),
	Archer UMETA(DisplayName = "Archer Tower"),
	Ice UMETA(DisplayName = "Ice / Frost Tower"),
	Paladin UMETA(DisplayName = "Paladin / Guardian Tower"),
};

/** Replaces placeholder cylinder meshes with imported Blender tower static meshes. */
UCLASS(ClassGroup = (Visual), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UTowerFabMeshComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTowerFabMeshComponent();

	UFUNCTION(BlueprintCallable, Category = "Tower|Mesh", meta = (DefaultToSelf = "Tower"))
	static UTowerFabMeshComponent* EnsureOn(AActor* Tower);

	UFUNCTION(BlueprintPure, Category = "Tower|Mesh")
	bool HasTowerMesh() const { return BodyMesh != nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Mesh")
	ETowerFabMeshKind MeshKind = ETowerFabMeshKind::Auto;

	/** Match height of /Engine/BasicShapes/Cylinder (100uu). Slightly taller for the princess. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Mesh", meta = (ClampMin = "1.0"))
	float TargetHeight = 125.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Mesh", meta = (ClampMin = "1.0"))
	float PrincessTargetHeight = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Mesh")
	float GroundZOffset = 0.f;

protected:
	virtual void BeginPlay() override;

	ETowerFabMeshKind ResolveKind() const;
	void HidePlaceholderMeshes();
	void ApplyTowerMesh(ETowerFabMeshKind Kind);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BodyMesh;
};
