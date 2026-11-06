#include "FrostAttackComponent.h"

#include "Components/StaticMeshComponent.h"
#include "EnemyStatusComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TowerFabMeshComponent.h"
#include "UnitReflectionUtils.h"

UFrostAttackComponent::UFrostAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFrostAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	UnitReflection::ApplyMaxHealthOverride(Owner, MaxHealthOverride);

	UTowerFabMeshComponent* TowerMesh = UTowerFabMeshComponent::EnsureOn(Owner);
	// Keep Blender ice-tower materials; only tint placeholder cylinders.
	if (TowerMesh && TowerMesh->HasTowerMesh())
	{
		return;
	}

	if (UMaterialInterface* BaseMaterial = UnitReflection::GetTintBaseMaterial())
	{
		BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), BodyColor);

		TArray<UStaticMeshComponent*> Meshes;
		UnitReflection::GetBodyMeshes(Owner, Meshes);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			if (!IsValid(Mesh) || !Mesh->IsVisible())
			{
				continue;
			}
			for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
			{
				Mesh->SetMaterial(Slot, BodyMaterial);
			}
		}
	}
}

void UFrostAttackComponent::ApplyFrostOnHit(AActor* Target) const
{
	if (UEnemyStatusComponent* Status = UEnemyStatusComponent::FindOrAddTo(Target))
	{
		Status->ApplySlow(SlowPercent, SlowDuration);
	}
}

void UFrostAttackComponent::TintProjectile(AActor* Projectile) const
{
	UMaterialInterface* BaseMaterial = UnitReflection::GetTintBaseMaterial();
	if (!IsValid(Projectile) || !BaseMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* ShotMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, Projectile);
	ShotMaterial->SetVectorParameterValue(TEXT("Color"), ProjectileColor);

	TArray<UStaticMeshComponent*> Meshes;
	UnitReflection::GetBodyMeshes(Projectile, Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
		{
			Mesh->SetMaterial(Slot, ShotMaterial);
		}
	}
}
