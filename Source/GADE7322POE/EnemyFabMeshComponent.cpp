#include "EnemyFabMeshComponent.h"

#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyFabMesh, Log, All);

namespace EnemyFabMeshPrivate
{
	static constexpr TCHAR GoblinMeshPath[] = TEXT("/Game/Fab/Stylized_Goblin_Model/goblin/SkeletalMeshes/goblin.goblin");
	static constexpr TCHAR GoblinWalkPath[] = TEXT("/Game/Fab/Stylized_Goblin_Model/goblin/SkeletalMeshes/goblinArmature_Walk.goblinArmature_Walk");
	static constexpr TCHAR NecromancerMeshPath[] = TEXT("/Game/Skeleton_Necromancer/Mesh/SKM_Skeleton_Necromancer.SKM_Skeleton_Necromancer");
	static constexpr TCHAR MinotaurMeshPath[] = TEXT("/Game/minotaur1/Mesh/sk_minotaur1.sk_minotaur1");
	static constexpr TCHAR MinotaurAxePath[] = TEXT("/Game/minotaur1/Mesh/sm_minotaur1_axe.sm_minotaur1_axe");
}

UEnemyFabMeshComponent::UEnemyFabMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UEnemyFabMeshComponent* UEnemyFabMeshComponent::EnsureOn(AActor* Enemy)
{
	if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed())
	{
		return nullptr;
	}
	if (UEnemyFabMeshComponent* Existing = Enemy->FindComponentByClass<UEnemyFabMeshComponent>())
	{
		return Existing;
	}

	UEnemyFabMeshComponent* Comp = NewObject<UEnemyFabMeshComponent>(Enemy, TEXT("EnemyFabMesh"));
	Enemy->AddInstanceComponent(Comp);
	Comp->RegisterComponent();
	return Comp;
}

EEnemyFabMeshKind UEnemyFabMeshComponent::ResolveKind() const
{
	if (MeshKind != EEnemyFabMeshKind::Auto)
	{
		return MeshKind;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return EEnemyFabMeshKind::Goblin;
	}

	const FString ClassName = Owner->GetClass()->GetName();
	if (ClassName.Contains(TEXT("Shaman")))
	{
		return EEnemyFabMeshKind::Shaman;
	}
	if (ClassName.Contains(TEXT("Berserker")))
	{
		return EEnemyFabMeshKind::Berserker;
	}
	return EEnemyFabMeshKind::Goblin;
}

void UEnemyFabMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	HidePlaceholderMeshes();
	ApplyFabMesh(ResolveKind());
}

void UEnemyFabMeshComponent::HidePlaceholderMeshes()
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
		if (!IsValid(Mesh))
		{
			continue;
		}
		const UStaticMesh* Asset = Mesh->GetStaticMesh();
		const FString MeshName = Mesh->GetName();
		const bool bLooksLikeBody =
			MeshName.Contains(TEXT("Mesh_GEN")) ||
			MeshName.Equals(TEXT("Mesh")) ||
			(Asset && (Asset->GetName().Contains(TEXT("Cylinder")) ||
				Asset->GetName().Contains(TEXT("Cube")) ||
				Asset->GetName().Contains(TEXT("Sphere"))));
		if (!bLooksLikeBody)
		{
			continue;
		}
		// Skip props that other components may have already added (staff/orb/aura/horn).
		if (MeshName.Contains(TEXT("Staff")) || MeshName.Contains(TEXT("Orb")) ||
			MeshName.Contains(TEXT("Aura")) || MeshName.Contains(TEXT("Horn")) ||
			MeshName.Contains(TEXT("Upgrade")) || MeshName.Contains(TEXT("Axe")))
		{
			continue;
		}
		Mesh->SetVisibility(false, true);
		Mesh->SetHiddenInGame(true, true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void UEnemyFabMeshComponent::ApplyFabMesh(EEnemyFabMeshKind Kind)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->GetRootComponent())
	{
		return;
	}
	if (BodyMesh)
	{
		return;
	}

	const TCHAR* MeshPath = EnemyFabMeshPrivate::GoblinMeshPath;
	float Scale = GoblinScale;
	switch (Kind)
	{
	case EEnemyFabMeshKind::Shaman:
		MeshPath = EnemyFabMeshPrivate::NecromancerMeshPath;
		Scale = ShamanScale;
		break;
	case EEnemyFabMeshKind::Berserker:
		MeshPath = EnemyFabMeshPrivate::MinotaurMeshPath;
		Scale = BerserkerScale;
		break;
	default:
		break;
	}

	USkeletalMesh* Skel = LoadObject<USkeletalMesh>(nullptr, MeshPath);
	if (!Skel)
	{
		UE_LOG(LogEnemyFabMesh, Error, TEXT("Failed to load Fab mesh %s for %s"), MeshPath, *GetNameSafe(Owner));
		return;
	}

	BodyMesh = NewObject<USkeletalMeshComponent>(Owner, TEXT("FabBodyMesh"));
	BodyMesh->SetSkeletalMesh(Skel);
	BodyMesh->SetupAttachment(Owner->GetRootComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCastShadow(true);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, GroundZOffset));
	BodyMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	BodyMesh->SetRelativeScale3D(FVector(Scale));
	BodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Owner->AddInstanceComponent(BodyMesh);
	BodyMesh->RegisterComponent();

	if (Kind == EEnemyFabMeshKind::Goblin)
	{
		if (UAnimationAsset* Walk = LoadObject<UAnimationAsset>(nullptr, EnemyFabMeshPrivate::GoblinWalkPath))
		{
			BodyMesh->PlayAnimation(Walk, true);
		}
	}

	if (Kind == EEnemyFabMeshKind::Berserker)
	{
		AttachMinotaurAxe();
	}

	UE_LOG(LogEnemyFabMesh, Warning, TEXT("%s using Fab mesh %s (scale %.2f)"),
		*GetNameSafe(Owner), *Skel->GetName(), Scale);
}

void UEnemyFabMeshComponent::AttachMinotaurAxe()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !BodyMesh)
	{
		return;
	}

	UStaticMesh* Axe = LoadObject<UStaticMesh>(nullptr, EnemyFabMeshPrivate::MinotaurAxePath);
	if (!Axe)
	{
		return;
	}

	AxeMesh = NewObject<UStaticMeshComponent>(Owner, TEXT("FabMinotaurAxe"));
	AxeMesh->SetStaticMesh(Axe);
	AxeMesh->SetupAttachment(BodyMesh);
	AxeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AxeMesh->SetCastShadow(true);
	// Approximate hand socket — packs vary; offset in character space.
	AxeMesh->SetRelativeLocation(FVector(40.f, -20.f, 80.f));
	AxeMesh->SetRelativeRotation(FRotator(0.f, 0.f, -40.f));
	AxeMesh->SetRelativeScale3D(FVector(1.f));
	Owner->AddInstanceComponent(AxeMesh);
	AxeMesh->RegisterComponent();
}
