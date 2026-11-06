#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyFabMeshComponent.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UAnimationAsset;

UENUM(BlueprintType)
enum class EEnemyFabMeshKind : uint8
{
	Auto UMETA(DisplayName = "Auto (from owner class)"),
	Goblin UMETA(DisplayName = "Stylized Goblin"),
	Shaman UMETA(DisplayName = "Skeleton Necromancer"),
	Berserker UMETA(DisplayName = "Minotaur"),
};

/** Replaces the placeholder cylinder body with Fab skeletal meshes. */
UCLASS(ClassGroup = (Visual), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UEnemyFabMeshComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyFabMeshComponent();

	/** Creates/applies Fab body mesh on any goblin-family enemy if missing. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Mesh", meta = (DefaultToSelf = "Enemy"))
	static UEnemyFabMeshComponent* EnsureOn(AActor* Enemy);

	UFUNCTION(BlueprintPure, Category = "Enemy|Mesh")
	USkeletalMeshComponent* GetBodyMesh() const { return BodyMesh; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Mesh")
	bool HasFabBody() const { return BodyMesh != nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Mesh")
	EEnemyFabMeshKind MeshKind = EEnemyFabMeshKind::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Mesh")
	float GoblinScale = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Mesh")
	float ShamanScale = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Mesh")
	float BerserkerScale = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Mesh")
	float GroundZOffset = 0.f;

protected:
	virtual void BeginPlay() override;

	EEnemyFabMeshKind ResolveKind() const;
	void HidePlaceholderMeshes();
	void ApplyFabMesh(EEnemyFabMeshKind Kind);
	void AttachMinotaurAxe();

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> AxeMesh;
};
