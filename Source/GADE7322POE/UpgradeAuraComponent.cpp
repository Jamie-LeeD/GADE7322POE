#include "UpgradeAuraComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UnitReflectionUtils.h"

UUpgradeAuraComponent::UUpgradeAuraComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUpgradeAuraComponent::BeginPlay()
{
	Super::BeginPlay();
	AuraMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Shaders/M_UpgradeAura.M_UpgradeAura"));
}

void UUpgradeAuraComponent::EnsureAuraMesh()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->GetRootComponent())
	{
		return;
	}

	if (!AuraMaterial)
	{
		AuraMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Shaders/M_UpgradeAura.M_UpgradeAura"));
	}

	if (!AuraMesh)
	{
		UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (!Sphere)
		{
			return;
		}

		AuraMesh = NewObject<UStaticMeshComponent>(Owner, TEXT("UpgradeAuraMesh"));
		AuraMesh->SetStaticMesh(Sphere);
		AuraMesh->SetupAttachment(Owner->GetRootComponent());
		AuraMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AuraMesh->SetGenerateOverlapEvents(false);
		AuraMesh->SetCastShadow(false);
		AuraMesh->SetCanEverAffectNavigation(false);
		Owner->AddInstanceComponent(AuraMesh);
		AuraMesh->RegisterComponent();
	}

	FVector Center;
	FVector Extent;
	UnitReflection::GetBodyBounds(Owner, Center, Extent);
	const float Radius = FMath::Max3(Extent.X, Extent.Y, Extent.Z);
	const float SphereRadius = 50.f; // Engine basic sphere radius
	const float Scale = FMath::Max(0.5f, (Radius * AuraScaleMultiplier) / SphereRadius);

	AuraMesh->SetRelativeLocation(Center + AuraOffset * 0.15f);
	AuraMesh->SetRelativeScale3D(FVector(Scale));
	if (AuraMaterial)
	{
		AuraMesh->SetMaterial(0, AuraMaterial);
	}
}

void UUpgradeAuraComponent::ShowAura()
{
	EnsureAuraMesh();
	if (AuraMesh)
	{
		AuraMesh->SetVisibility(true, true);
		bAuraVisible = true;
	}
}

void UUpgradeAuraComponent::HideAura()
{
	if (AuraMesh)
	{
		AuraMesh->SetVisibility(false, true);
	}
	bAuraVisible = false;
}

UUpgradeAuraComponent* UUpgradeAuraComponent::ApplyTo(AActor* Defender)
{
	if (!IsValid(Defender) || Defender->IsActorBeingDestroyed())
	{
		return nullptr;
	}

	UUpgradeAuraComponent* Aura = Defender->FindComponentByClass<UUpgradeAuraComponent>();
	if (!Aura)
	{
		Aura = NewObject<UUpgradeAuraComponent>(Defender, TEXT("UpgradeAura"));
		Defender->AddInstanceComponent(Aura);
		Aura->RegisterComponent();
	}
	Aura->ShowAura();
	return Aura;
}
