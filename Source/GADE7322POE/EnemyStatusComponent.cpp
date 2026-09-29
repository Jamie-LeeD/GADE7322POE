#include "EnemyStatusComponent.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UnitReflectionUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyStatus, Log, All);

UEnemyStatusComponent::UEnemyStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

UEnemyStatusComponent* UEnemyStatusComponent::FindOrAddTo(AActor* Enemy)
{
	if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed())
	{
		return nullptr;
	}
	if (UEnemyStatusComponent* Existing = Enemy->FindComponentByClass<UEnemyStatusComponent>())
	{
		return Existing;
	}

	UEnemyStatusComponent* Status = NewObject<UEnemyStatusComponent>(
		Enemy, MakeUniqueObjectName(Enemy, UEnemyStatusComponent::StaticClass(), TEXT("EnemyStatus")));
	Enemy->AddInstanceComponent(Status);
	Status->RegisterComponent();
	return Status;
}

void UEnemyStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	CaptureBaseSpeed();
}

void UEnemyStatusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AActor* Owner = GetOwner();
	if (IsValid(Owner) && !Owner->IsActorBeingDestroyed() && BaseMoveSpeed >= 0.f)
	{
		UnitReflection::SetNumeric(Owner, SpeedPropertyName, BaseMoveSpeed);
	}
	RestoreMaterials();
	Super::EndPlay(EndPlayReason);
}

void UEnemyStatusComponent::CaptureBaseSpeed()
{
	if (BaseMoveSpeed < 0.f && UnitReflection::HasNumeric(GetOwner(), SpeedPropertyName))
	{
		BaseMoveSpeed = UnitReflection::GetNumeric(GetOwner(), SpeedPropertyName, 0.f);
		LastWrittenSpeed = BaseMoveSpeed;
	}
}

void UEnemyStatusComponent::ApplySpeed()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !UnitReflection::HasNumeric(Owner, SpeedPropertyName))
	{
		return;
	}

	const float Current = UnitReflection::GetNumeric(Owner, SpeedPropertyName, 0.f);
	if (BaseMoveSpeed < 0.f)
	{
		BaseMoveSpeed = Current;
	}
	else if (LastWrittenSpeed >= 0.f && !FMath::IsNearlyEqual(Current, LastWrittenSpeed, 0.01f))
	{
		// The Blueprint changed MoveSpeed itself (e.g. stop at tower); treat that as the new normal speed.
		BaseMoveSpeed = Current;
	}

	const float Desired = FMath::Max(0.f, BaseMoveSpeed * SlowMultiplier * SpeedBoostMultiplier);
	if (!FMath::IsNearlyEqual(Current, Desired, 0.01f))
	{
		UnitReflection::SetNumeric(Owner, SpeedPropertyName, Desired);
	}
	LastWrittenSpeed = Desired;
}

void UEnemyStatusComponent::ApplySlow(float SlowPercent, float Duration)
{
	SlowPercent = FMath::Clamp(SlowPercent, 0.f, 0.95f);
	if (SlowPercent <= 0.f || Duration <= 0.f)
	{
		return;
	}

	CaptureBaseSpeed();

	const bool bWasSlowed = IsSlowed();
	const float NewMultiplier = 1.f - SlowPercent;

	// Refresh instead of stack: keep the strongest single slow and the longest remaining time.
	SlowMultiplier = bWasSlowed ? FMath::Min(SlowMultiplier, NewMultiplier) : NewMultiplier;
	SlowTimeRemaining = FMath::Max(SlowTimeRemaining, Duration);

	ApplySpeed();
	RefreshVisual();

	if (!bWasSlowed)
	{
		UE_LOG(LogEnemyStatus, Log, TEXT("%s slowed %.0f%% for %.1fs (speed %.0f -> %.0f)"),
			*GetNameSafe(GetOwner()), SlowPercent * 100.f, Duration, BaseMoveSpeed, LastWrittenSpeed);
		OnSlowChanged.Broadcast(true);
	}
}

void UEnemyStatusComponent::ClearSlow()
{
	const bool bWasSlowed = IsSlowed();
	SlowTimeRemaining = 0.f;
	SlowMultiplier = 1.f;
	ApplySpeed();
	RefreshVisual();
	if (bWasSlowed)
	{
		OnSlowChanged.Broadcast(false);
	}
}

void UEnemyStatusComponent::SetSpeedBoost(float Multiplier)
{
	CaptureBaseSpeed();
	SpeedBoostMultiplier = FMath::Max(0.f, Multiplier);
	ApplySpeed();
}

void UEnemyStatusComponent::SetBodyTint(bool bEnable, FLinearColor Color)
{
	bHasBodyTint = bEnable;
	BodyTint = Color;
	RefreshVisual();
}

void UEnemyStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SlowTimeRemaining > 0.f)
	{
		SlowTimeRemaining -= DeltaTime;
		if (SlowTimeRemaining <= 0.f)
		{
			ClearSlow();
			return;
		}
	}

	ApplySpeed();
}

void UEnemyStatusComponent::RefreshVisual()
{
	const bool bSlowed = IsSlowed();
	if (!bSlowed && !bHasBodyTint)
	{
		RestoreMaterials();
		return;
	}

	FLinearColor Color = SlowedTint;
	if (bHasBodyTint)
	{
		Color = bSlowed ? FLinearColor::LerpUsingHSV(BodyTint, SlowedTint, SlowedTintStrength) : BodyTint;
	}
	if (bVisualApplied && Color.Equals(LastAppliedColor))
	{
		return;
	}

	UMaterialInterface* BaseMaterial = UnitReflection::GetTintBaseMaterial();
	if (!BaseMaterial)
	{
		return;
	}

	if (SavedMaterials.Num() == 0)
	{
		TArray<UStaticMeshComponent*> Meshes;
		UnitReflection::GetBodyMeshes(GetOwner(), Meshes);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			FStatusSavedMeshMaterials& Entry = SavedMaterials.AddDefaulted_GetRef();
			Entry.Mesh = Mesh;
			for (UMaterialInterface* Material : Mesh->GetMaterials())
			{
				Entry.Materials.Add(Material);
			}
			Entry.TintMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		}
	}

	for (FStatusSavedMeshMaterials& Entry : SavedMaterials)
	{
		if (!IsValid(Entry.Mesh) || !Entry.TintMaterial)
		{
			continue;
		}
		Entry.TintMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		for (int32 Slot = 0; Slot < Entry.Mesh->GetNumMaterials(); ++Slot)
		{
			Entry.Mesh->SetMaterial(Slot, Entry.TintMaterial);
		}
	}

	bVisualApplied = true;
	LastAppliedColor = Color;
}

void UEnemyStatusComponent::RestoreMaterials()
{
	if (!bVisualApplied)
	{
		return;
	}
	for (FStatusSavedMeshMaterials& Entry : SavedMaterials)
	{
		if (!IsValid(Entry.Mesh))
		{
			continue;
		}
		for (int32 Slot = 0; Slot < Entry.Materials.Num(); ++Slot)
		{
			Entry.Mesh->SetMaterial(Slot, Entry.Materials[Slot]);
		}
	}
	bVisualApplied = false;
}
