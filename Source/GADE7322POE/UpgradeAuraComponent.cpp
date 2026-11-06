#include "UpgradeAuraComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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

FLinearColor UUpgradeAuraComponent::GetColorForLevel(int32 Level) const
{
	return Level >= 2 ? Level2Color : Level1Color;
}

void UUpgradeAuraComponent::ApplyAuraColor()
{
	if (!AuraMesh)
	{
		return;
	}
	if (!AuraMID && AuraMaterial)
	{
		AuraMID = UMaterialInstanceDynamic::Create(AuraMaterial, this);
	}
	if (AuraMID)
	{
		AuraMID->SetVectorParameterValue(TEXT("AuraColor"), GetColorForLevel(UpgradeLevel));
		AuraMesh->SetMaterial(0, AuraMID);
	}
	else if (AuraMaterial)
	{
		AuraMesh->SetMaterial(0, AuraMaterial);
	}
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
		// Keep aura size independent of defender actor scale changes on upgrade.
		AuraMesh->SetAbsolute(false, false, true);
		Owner->AddInstanceComponent(AuraMesh);
		AuraMesh->RegisterComponent();
	}

	FVector Center;
	FVector Extent;
	UnitReflection::GetBodyBounds(Owner, Center, Extent);
	AuraMesh->SetRelativeLocation(Center + AuraOffset * 0.15f);

	if (!bAuraScaleLocked)
	{
		const float Radius = FMath::Max3(Extent.X, Extent.Y, Extent.Z);
		const float SphereRadius = 50.f;
		LockedAuraScale = FMath::Max(0.5f, (Radius * AuraScaleMultiplier) / SphereRadius);
		bAuraScaleLocked = true;
	}
	AuraMesh->SetWorldScale3D(FVector(LockedAuraScale));
	ApplyAuraColor();
}

void UUpgradeAuraComponent::SetUpgradeLevel(int32 InUpgradeLevel)
{
	UpgradeLevel = FMath::Max(1, InUpgradeLevel);
	if (bAuraVisible)
	{
		EnsureAuraMesh();
		ApplyAuraColor();
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

UUpgradeAuraComponent* UUpgradeAuraComponent::ApplyTo(AActor* Defender, int32 InUpgradeLevel)
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
	Aura->SetUpgradeLevel(InUpgradeLevel);
	Aura->ShowAura();
	return Aura;
}
