#include "Units/BerserkerRageComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Units/EnemyStatusComponent.h"
#include "GameFramework/Actor.h"
#include "Units/UnitReflectionUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogBerserker, Log, All);

UBerserkerRageComponent::UBerserkerRageComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UBerserkerRageComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	UnitReflection::ApplyMaxHealthOverride(Owner, MaxHealthOverride);

	TArray<UStaticMeshComponent*> Meshes;
	UnitReflection::GetBodyMeshes(Owner, Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		BodyMeshes.Add(Mesh);
		OriginalMeshScales.Add(Mesh->GetRelativeScale3D());
	}
	ApplyMeshScale(0.f);

	Status = UEnemyStatusComponent::FindOrAddTo(Owner);
	if (Status)
	{
		Status->SetBodyTint(true, BodyColor);
	}
}

float UBerserkerRageComponent::GetHealthPercent() const
{
	const UActorComponent* Health = UnitReflection::FindHealthComponent(GetOwner());
	if (!Health)
	{
		return 1.f;
	}
	const float Max = UnitReflection::GetNumeric(Health, FName(TEXT("MaxHealth")), 0.f);
	const float Current = UnitReflection::GetNumeric(Health, FName(TEXT("CurrentHealth")), Max);
	return Max > 0.f ? Current / Max : 1.f;
}

void UBerserkerRageComponent::ApplyMeshScale(float PulseAlpha)
{
	const float Pulse = 1.f + RagePulseScale * PulseAlpha;
	for (int32 Index = 0; Index < BodyMeshes.Num(); ++Index)
	{
		if (IsValid(BodyMeshes[Index]))
		{
			BodyMeshes[Index]->SetRelativeScale3D(OriginalMeshScales[Index] * BodyScale * Pulse);
		}
	}
}

void UBerserkerRageComponent::StartRage()
{
	AActor* Owner = GetOwner();
	if (bIsRaging || !IsValid(Owner))
	{
		return;
	}

	bIsRaging = true;
	RageTimeRemaining = RageDuration;
	RageAge = 0.f;

	if (!Status)
	{
		Status = UEnemyStatusComponent::FindOrAddTo(Owner);
	}
	if (Status)
	{
		Status->SetSpeedBoost(RageSpeedMultiplier);
		Status->SetDamageBoost(RageDamageMultiplier);
		Status->SetBodyTint(true, RageColor);
	}

	UE_LOG(LogBerserker, Warning, TEXT("%s RAGE! %.1fs, speed x%.2f, damage x%.2f"),
		*GetNameSafe(Owner), RageDuration, RageSpeedMultiplier, RageDamageMultiplier);
	OnRageChanged.Broadcast(true);
}

void UBerserkerRageComponent::EndRage()
{
	if (!bIsRaging)
	{
		return;
	}

	bIsRaging = false;
	RageTimeRemaining = 0.f;
	CooldownRemaining = RageCooldown;

	AActor* Owner = GetOwner();
	if (Status)
	{
		Status->SetSpeedBoost(1.f);
		Status->SetDamageBoost(1.f);
		Status->SetBodyTint(true, BodyColor);
	}
	ApplyMeshScale(0.f);

	UE_LOG(LogBerserker, Log, TEXT("%s rage ended (cooldown %.1fs)"), *GetNameSafe(Owner), RageCooldown);
	OnRageChanged.Broadcast(false);
}

void UBerserkerRageComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsRaging)
	{
		RageAge += DeltaTime;
		RageTimeRemaining -= DeltaTime;
		ApplyMeshScale(0.5f + 0.5f * FMath::Sin(RageAge * 12.f));
		if (RageTimeRemaining <= 0.f)
		{
			EndRage();
		}
		return;
	}

	if (CooldownRemaining > 0.f)
	{
		CooldownRemaining -= DeltaTime;
		return;
	}

	const float Percent = GetHealthPercent();
	if (Percent > 0.f && Percent <= RageHealthThreshold)
	{
		StartRage();
	}
}
