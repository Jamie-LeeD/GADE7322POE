#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralEliteComponent.generated.h"

class UStaticMeshComponent;

/**
 * Jamie Part 4/5 — seeded elite phenotype: scale, body tint, optional horn,
 * plus health / speed / gold multipliers stacked on the spawned goblin.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UProceduralEliteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UProceduralEliteComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elite")
	int32 EliteSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Elite")
	float RolledScale = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Elite")
	FLinearColor RolledColour = FLinearColor::Green;

	UPROPERTY(BlueprintReadOnly, Category = "Elite")
	float RolledHealthMult = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Elite")
	float RolledSpeedMult = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Elite")
	float RolledGoldMult = 1.f;

	void ApplyPhenotype();

protected:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HornMesh;
};
