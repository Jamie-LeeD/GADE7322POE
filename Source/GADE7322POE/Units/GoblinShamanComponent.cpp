#include "Units/GoblinShamanComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Units/EnemyStatusComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Units/UnitReflectionUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoblinShaman, Log, All);

namespace GoblinShamanPrivate
{
	static constexpr float AuraDuration = 0.45f;
}

UGoblinShamanComponent::UGoblinShamanComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UGoblinShamanComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	UnitReflection::ApplyMaxHealthOverride(Owner, MaxHealthOverride);
	GoblinClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinEnemy.BP_GoblinEnemy_C"));

	// Body colour goes through the status component so Frost slows / other buffs still layer on top.
	// Done before adding the staff so only the original body mesh is recoloured.
	if (UEnemyStatusComponent* Status = UEnemyStatusComponent::FindOrAddTo(Owner))
	{
		Status->SetBodyTint(true, BodyColor);
	}

	FVector Center;
	FVector Extent;
	UnitReflection::GetBodyBounds(Owner, Center, Extent);
	const float Radius = FMath::Max(Extent.X, Extent.Y);
	const float StaffHeight = Extent.Z * 2.f + 40.f;

	StaffMesh = UnitReflection::AddVisualShape(Owner, TEXT("Cylinder"), StaffColor,
		Center + FVector(10.f, Radius + 12.f, 20.f), FRotator::ZeroRotator, FVector(0.07f, 0.07f, StaffHeight / 100.f));
	OrbRestLocation = Center + FVector(10.f, Radius + 12.f, 20.f + StaffHeight * 0.5f + 14.f);
	OrbMesh = UnitReflection::AddVisualShape(Owner, TEXT("Sphere"), MagicColor, OrbRestLocation, FRotator::ZeroRotator, FVector(0.3f));

	AuraFullScale = FVector(SupportRadius * 2.f / 100.f, SupportRadius * 2.f / 100.f, 0.01f);
	AuraMesh = UnitReflection::AddVisualShape(Owner, TEXT("Cylinder"), MagicColor,
		FVector(Center.X, Center.Y, Center.Z - Extent.Z + 3.f), FRotator::ZeroRotator, AuraFullScale);
	if (AuraMesh)
	{
		AuraMesh->SetVisibility(false);
	}
}

bool UGoblinShamanComponent::IsAllyAlive(const AActor* Ally) const
{
	if (!IsValid(Ally) || Ally->IsActorBeingDestroyed() || Ally->IsHidden())
	{
		return false;
	}
	if (const FBoolProperty* DeadProp = FindFProperty<FBoolProperty>(Ally->GetClass(), FName(TEXT("bIsDead"))))
	{
		if (DeadProp->GetPropertyValue_InContainer(Ally))
		{
			return false;
		}
	}
	if (const UActorComponent* Health = UnitReflection::FindHealthComponent(Ally))
	{
		return UnitReflection::GetNumeric(Health, FName(TEXT("CurrentHealth")), 1.f) > 0.f;
	}
	return true;
}

int32 UGoblinShamanComponent::CastSupportPulse()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !World || !GoblinClass)
	{
		return 0;
	}

	int32 Buffed = 0;
	const float RadiusSq = FMath::Square(SupportRadius);
	for (TActorIterator<AActor> It(World, GoblinClass); It; ++It)
	{
		AActor* Ally = *It;
		if (Ally == Owner || !IsAllyAlive(Ally) ||
			FVector::DistSquared2D(Ally->GetActorLocation(), Owner->GetActorLocation()) > RadiusSq)
		{
			continue;
		}
		if (UEnemyStatusComponent* Status = UEnemyStatusComponent::FindOrAddTo(Ally))
		{
			Status->ApplySupportBuff(BuffSpeedBonus, BuffDamageBonus, BuffDuration);
			++Buffed;
		}
	}

	AuraTimeRemaining = GoblinShamanPrivate::AuraDuration;
	if (AuraMesh)
	{
		AuraMesh->SetVisibility(true);
	}
	if (Buffed > 0)
	{
		UE_LOG(LogGoblinShaman, Log, TEXT("%s empowered %d allies (+%.0f%% speed, +%.0f%% damage, %.1fs)"),
			*GetNameSafe(Owner), Buffed, BuffSpeedBonus * 100.f, BuffDamageBonus * 100.f, BuffDuration);
	}
	OnSupportPulse.Broadcast(Buffed);
	return Buffed;
}

void UGoblinShamanComponent::TintProjectile(AActor* Projectile) const
{
	UMaterialInterface* Base = UnitReflection::GetTintBaseMaterial();
	if (!IsValid(Projectile) || !Base)
	{
		return;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Projectile);
	MID->SetVectorParameterValue(TEXT("Color"), MagicColor);
	TArray<UStaticMeshComponent*> Meshes;
	UnitReflection::GetBodyMeshes(Projectile, Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
		{
			Mesh->SetMaterial(Slot, MID);
		}
	}
}

void UGoblinShamanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Age += DeltaTime;
	if (OrbMesh)
	{
		OrbMesh->SetRelativeLocation(OrbRestLocation + FVector(0.f, 0.f, 8.f * FMath::Sin(Age * 4.f)));
	}

	if (AuraTimeRemaining > 0.f && AuraMesh)
	{
		AuraTimeRemaining -= DeltaTime;
		const float Alpha = 1.f - FMath::Max(0.f, AuraTimeRemaining) / GoblinShamanPrivate::AuraDuration;
		AuraMesh->SetRelativeScale3D(FVector(AuraFullScale.X * Alpha, AuraFullScale.Y * Alpha, AuraFullScale.Z));
		if (AuraTimeRemaining <= 0.f)
		{
			AuraMesh->SetVisibility(false);
		}
	}

	PulseCooldown -= DeltaTime;
	if (PulseCooldown <= 0.f)
	{
		PulseCooldown = SupportInterval;
		CastSupportPulse();
	}
}
