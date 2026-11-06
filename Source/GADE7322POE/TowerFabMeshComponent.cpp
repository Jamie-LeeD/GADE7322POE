#include "TowerFabMeshComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogTowerFabMesh, Log, All);

namespace TowerFabMeshPrivate
{
	static constexpr TCHAR PrincessPath[] = TEXT("/Game/Imports/Towers/PrincessTower.PrincessTower");
	static constexpr TCHAR ArcherPath[] = TEXT("/Game/Imports/Towers/ArcherTower.ArcherTower");
	static constexpr TCHAR IcePath[] = TEXT("/Game/Imports/Towers/IceTower.IceTower");
	static constexpr TCHAR PaladinPath[] = TEXT("/Game/Imports/Towers/PaladinTower.PaladinTower");
}

UTowerFabMeshComponent::UTowerFabMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UTowerFabMeshComponent* UTowerFabMeshComponent::EnsureOn(AActor* Tower)
{
	if (!IsValid(Tower) || Tower->IsActorBeingDestroyed())
	{
		return nullptr;
	}
	if (UTowerFabMeshComponent* Existing = Tower->FindComponentByClass<UTowerFabMeshComponent>())
	{
		return Existing;
	}
	UTowerFabMeshComponent* Comp = NewObject<UTowerFabMeshComponent>(Tower, TEXT("TowerFabMesh"));
	Tower->AddInstanceComponent(Comp);
	Comp->RegisterComponent();
	return Comp;
}

ETowerFabMeshKind UTowerFabMeshComponent::ResolveKind() const
{
	if (MeshKind != ETowerFabMeshKind::Auto)
	{
		return MeshKind;
	}
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return ETowerFabMeshKind::Archer;
	}
	const FString Name = Owner->GetClass()->GetName();
	if (Name.Contains(TEXT("Princess")))
	{
		return ETowerFabMeshKind::Princess;
	}
	if (Name.Contains(TEXT("Frost")))
	{
		return ETowerFabMeshKind::Ice;
	}
	if (Name.Contains(TEXT("Guardian")) || Name.Contains(TEXT("Paladin")))
	{
		return ETowerFabMeshKind::Paladin;
	}
	if (Name.Contains(TEXT("Archer")) || Name.Contains(TEXT("RoyalArcher")))
	{
		return ETowerFabMeshKind::Archer;
	}
	return ETowerFabMeshKind::Archer;
}

void UTowerFabMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	HidePlaceholderMeshes();
	ApplyTowerMesh(ResolveKind());
}

void UTowerFabMeshComponent::HidePlaceholderMeshes()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}
	TArray<UStaticMeshComponent*> Meshes;
	Owner->GetComponents(Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh) || Mesh == BodyMesh)
		{
			continue;
		}
		const UStaticMesh* Asset = Mesh->GetStaticMesh();
		const FString MeshName = Mesh->GetName();
		const bool bLooksLikePlaceholder =
			MeshName.Contains(TEXT("Mesh_GEN")) ||
			MeshName.Equals(TEXT("Mesh")) ||
			MeshName.Contains(TEXT("Base")) ||
			(Asset && (Asset->GetName().Contains(TEXT("Cylinder")) ||
				Asset->GetName().Contains(TEXT("Cube")) ||
				Asset->GetName().Contains(TEXT("Sphere")) ||
				Asset->GetName().Contains(TEXT("Cone"))));
		if (!bLooksLikePlaceholder)
		{
			continue;
		}
		if (MeshName.Contains(TEXT("Shield")) || MeshName.Contains(TEXT("Sword")) ||
			MeshName.Contains(TEXT("Aura")) || MeshName.Contains(TEXT("Upgrade")) ||
			MeshName.Contains(TEXT("Fab")) || MeshName.Contains(TEXT("TowerBody")))
		{
			continue;
		}
		Mesh->SetVisibility(false, true);
		Mesh->SetHiddenInGame(true, true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void UTowerFabMeshComponent::ApplyTowerMesh(ETowerFabMeshKind Kind)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->GetRootComponent() || BodyMesh)
	{
		return;
	}

	const TCHAR* MeshPath = TowerFabMeshPrivate::ArcherPath;
	float DesiredHeight = TargetHeight;
	switch (Kind)
	{
	case ETowerFabMeshKind::Princess:
		MeshPath = TowerFabMeshPrivate::PrincessPath;
		DesiredHeight = PrincessTargetHeight;
		break;
	case ETowerFabMeshKind::Ice:
		MeshPath = TowerFabMeshPrivate::IcePath;
		break;
	case ETowerFabMeshKind::Paladin:
		MeshPath = TowerFabMeshPrivate::PaladinPath;
		break;
	default:
		break;
	}

	UStaticMesh* MeshAsset = LoadObject<UStaticMesh>(nullptr, MeshPath);
	if (!MeshAsset)
	{
		UE_LOG(LogTowerFabMesh, Error, TEXT("Failed to load tower mesh %s for %s"), MeshPath, *GetNameSafe(Owner));
		return;
	}

	// Uniform scale so imported mesh height matches the old prototype cylinder (~100uu).
	const float MeshHeight = FMath::Max(1.f, MeshAsset->GetBounds().BoxExtent.Z * 2.f);
	const float Scale = DesiredHeight / MeshHeight;

	BodyMesh = NewObject<UStaticMeshComponent>(Owner, TEXT("TowerBodyMesh"));
	BodyMesh->SetStaticMesh(MeshAsset);
	BodyMesh->SetupAttachment(Owner->GetRootComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCastShadow(true);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, GroundZOffset));
	BodyMesh->SetRelativeRotation(FRotator::ZeroRotator);
	BodyMesh->SetRelativeScale3D(FVector(Scale));
	Owner->AddInstanceComponent(BodyMesh);
	BodyMesh->RegisterComponent();

	UE_LOG(LogTowerFabMesh, Warning, TEXT("%s using tower mesh %s (nativeH=%.0f -> scale %.3f for targetH=%.0f)"),
		*GetNameSafe(Owner), *MeshAsset->GetName(), MeshHeight, Scale, DesiredHeight);
}
