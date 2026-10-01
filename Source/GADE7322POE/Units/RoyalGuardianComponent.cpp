#include "Units/RoyalGuardianComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Units/UnitReflectionUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoyalGuardian, Log, All);

namespace RoyalGuardianPrivate
{
	static constexpr float SwingDuration = 0.3f;
}

URoyalGuardianComponent::URoyalGuardianComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void URoyalGuardianComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	UnitReflection::ApplyMaxHealthOverride(Owner, MaxHealthOverride);
	EnemyClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinEnemy.BP_GoblinEnemy_C"));

	
	TArray<UStaticMeshComponent*> Meshes;
	UnitReflection::GetBodyMeshes(Owner, Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		BodyMeshes.Add(Mesh);
		Mesh->SetRelativeScale3D(Mesh->GetRelativeScale3D() * BodyScale);
	}
	ApplyBodyColor(BodyColor);

	FVector Center;
	FVector Extent;
	UnitReflection::GetBodyBounds(Owner, Center, Extent);
	const float Radius = FMath::Max(Extent.X, Extent.Y);

	
	ShieldRestLocation = Center + FVector(Radius + 6.f, 0.f, 0.f);
	ShieldRestScale = FVector(0.95f, 0.95f, 0.12f);
	ShieldMesh = UnitReflection::AddVisualShape(Owner, TEXT("Cylinder"), ShieldColor, ShieldRestLocation, FRotator(90.f, 0.f, 0.f), ShieldRestScale);

	SwordRestRotation = FRotator(0.f, 0.f, 0.f);
	SwordMesh = UnitReflection::AddVisualShape(Owner, TEXT("Cube"), SwordColor,
		Center + FVector(10.f, Radius + 14.f, Extent.Z * 0.35f), SwordRestRotation, FVector(0.08f, 0.14f, 1.1f));
}

float URoyalGuardianComponent::GetIncomingDamageMultiplier() const
{
	return bShieldGuardActive ? (1.f - ShieldDamageReduction) : 1.f;
}

void URoyalGuardianComponent::ApplyBodyColor(const FLinearColor& Color)
{
	for (UStaticMeshComponent* Mesh : BodyMeshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}
		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		if (!MID)
		{
			if (UMaterialInterface* Base = UnitReflection::GetTintBaseMaterial())
			{
				MID = UMaterialInstanceDynamic::Create(Base, this);
				for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
				{
					Mesh->SetMaterial(Slot, MID);
				}
			}
		}
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}

void URoyalGuardianComponent::UpdateShieldPose()
{
	if (!ShieldMesh)
	{
		return;
	}
	// Raised shield: pushed forward, larger and glowing.
	ShieldMesh->SetRelativeLocation(bShieldGuardActive ? ShieldRestLocation + FVector(18.f, 0.f, 10.f) : ShieldRestLocation);
	ShieldMesh->SetRelativeScale3D(bShieldGuardActive ? ShieldRestScale * FVector(1.5f, 1.5f, 1.f) : ShieldRestScale);
	UnitReflection::SetShapeColor(ShieldMesh, bShieldGuardActive ? ShieldActiveColor : ShieldColor);
}

void URoyalGuardianComponent::ActivateShieldGuard()
{
	if (bShieldGuardActive)
	{
		return;
	}
	bShieldGuardActive = true;
	ShieldTimeRemaining = ShieldDuration;
	ApplyBodyColor(ShieldGuardBodyColor);
	UpdateShieldPose();
	UE_LOG(LogRoyalGuardian, Warning, TEXT("%s SHIELD GUARD up (%.0f%% less damage for %.1fs)"),
		*GetNameSafe(GetOwner()), ShieldDamageReduction * 100.f, ShieldDuration);
	OnShieldGuardChanged.Broadcast(true);
}

void URoyalGuardianComponent::EndShieldGuard()
{
	if (!bShieldGuardActive)
	{
		return;
	}
	bShieldGuardActive = false;
	ShieldTimeRemaining = 0.f;
	ShieldCooldownRemaining = ShieldCooldown;
	ApplyBodyColor(BodyColor);
	UpdateShieldPose();
	UE_LOG(LogRoyalGuardian, Log, TEXT("%s shield guard down (cooldown %.1fs)"), *GetNameSafe(GetOwner()), ShieldCooldown);
	OnShieldGuardChanged.Broadcast(false);
}

void URoyalGuardianComponent::NotifyMeleeStrike(AActor* Target)
{
	SwingTimeRemaining = RoyalGuardianPrivate::SwingDuration;
	if (AActor* Owner = GetOwner(); IsValid(Owner) && IsValid(Target))
	{
		FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
		ToTarget.Z = 0.f;
		if (!ToTarget.IsNearlyZero())
		{
			Owner->SetActorRotation(ToTarget.Rotation());
		}
	}
}

bool URoyalGuardianComponent::IsEnemyNearby() const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !EnemyClass)
	{
		return false;
	}
	const float RangeSq = FMath::Square(ShieldTriggerRange);
	for (TActorIterator<AActor> It(World, EnemyClass); It; ++It)
	{
		if (IsValid(*It) && !It->IsHidden() && FVector::DistSquared(It->GetActorLocation(), Owner->GetActorLocation()) <= RangeSq)
		{
			return true;
		}
	}
	return false;
}

void URoyalGuardianComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SwingTimeRemaining > 0.f && SwordMesh)
	{
		SwingTimeRemaining = FMath::Max(0.f, SwingTimeRemaining - DeltaTime);
		const float Alpha = FMath::Sin((1.f - SwingTimeRemaining / RoyalGuardianPrivate::SwingDuration) * PI);
		SwordMesh->SetRelativeRotation(SwordRestRotation + FRotator(-75.f * Alpha, 0.f, 0.f));
	}

	if (bShieldGuardActive)
	{
		ShieldTimeRemaining -= DeltaTime;
		if (ShieldTimeRemaining <= 0.f)
		{
			EndShieldGuard();
		}
		return;
	}

	if (ShieldCooldownRemaining > 0.f)
	{
		ShieldCooldownRemaining -= DeltaTime;
		return;
	}

	EnemyCheckCooldown -= DeltaTime;
	if (EnemyCheckCooldown <= 0.f)
	{
		EnemyCheckCooldown = 0.2f;
		if (IsEnemyNearby())
		{
			ActivateShieldGuard();
		}
	}
}
