#include "ProceduralEliteComponent.h"

#include "EnemyStatusComponent.h"
#include "GameFramework/Actor.h"
#include "UnitReflectionUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogProceduralElite, Log, All);

namespace ProceduralElitePrivate
{
	static const FLinearColor Palette[] = {
		FLinearColor(0.15f, 0.85f, 0.20f), // toxic green
		FLinearColor(0.55f, 0.20f, 0.75f), // bruise purple
		FLinearColor(0.85f, 0.35f, 0.10f), // rust orange
		FLinearColor(0.45f, 0.48f, 0.50f), // ash grey
		FLinearColor(0.75f, 0.12f, 0.15f), // blood red
	};
	static constexpr int32 PaletteCount = UE_ARRAY_COUNT(Palette);
}

UProceduralEliteComponent::UProceduralEliteComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UProceduralEliteComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyPhenotype();
}

void UProceduralEliteComponent::ApplyPhenotype()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}
	if (EliteSeed == 0)
	{
		EliteSeed = 1;
	}

	FRandomStream Stream(EliteSeed);
	RolledScale = Stream.FRandRange(1.15f, 1.45f);
	RolledColour = ProceduralElitePrivate::Palette[Stream.RandRange(0, ProceduralElitePrivate::PaletteCount - 1)];
	RolledHealthMult = Stream.FRandRange(1.4f, 2.2f);
	RolledSpeedMult = Stream.FRandRange(0.85f, 1.15f);
	RolledGoldMult = Stream.FRandRange(1.5f, 2.5f);
	const bool bAddHorn = Stream.FRand() < 0.65f;

	Owner->SetActorScale3D(Owner->GetActorScale3D() * RolledScale);

	if (UEnemyStatusComponent* Status = UEnemyStatusComponent::FindOrAddTo(Owner))
	{
		Status->SetBodyTint(true, RolledColour);
	}

	const FName MaxHealthName(TEXT("MaxHealth"));
	const FName CurrentHealthName(TEXT("CurrentHealth"));
	if (UActorComponent* Health = UnitReflection::FindHealthComponent(Owner))
	{
		const float NewMax = UnitReflection::GetNumeric(Health, MaxHealthName, 100.f) * RolledHealthMult;
		UnitReflection::SetNumeric(Health, MaxHealthName, NewMax);
		UnitReflection::SetNumeric(Health, CurrentHealthName, NewMax);
		if (UnitReflection::HasNumeric(Owner, CurrentHealthName))
		{
			UnitReflection::SetNumeric(Owner, MaxHealthName, NewMax);
			UnitReflection::SetNumeric(Owner, CurrentHealthName, NewMax);
		}
		if (UFunction* Update = Health->FindFunction(FName(TEXT("UpdateHealthDisplay"))))
		{
			Health->ProcessEvent(Update, nullptr);
		}
	}

	const FName MoveSpeedName(TEXT("MoveSpeed"));
	UnitReflection::SetNumeric(Owner, MoveSpeedName,
		UnitReflection::GetNumeric(Owner, MoveSpeedName, 0.f) * RolledSpeedMult);

	const FName GoldName(TEXT("GoldReward"));
	UnitReflection::SetNumeric(Owner, GoldName,
		UnitReflection::GetNumeric(Owner, GoldName, 25.f) * RolledGoldMult);

	if (bAddHorn)
	{
		FVector Center, Extent;
		UnitReflection::GetBodyBounds(Owner, Center, Extent);
		HornMesh = UnitReflection::AddVisualShape(
			Owner,
			TEXT("Cone"),
			RolledColour * 1.15f,
			Center + FVector(0.f, 0.f, Extent.Z * 0.55f),
			FRotator(0.f, 0.f, 180.f),
			FVector(0.35f, 0.35f, 0.55f));
	}

	UE_LOG(LogProceduralElite, Log,
		TEXT("Elite seed=%d scale=%.2f colour=(%.2f,%.2f,%.2f) HP x%.2f Spd x%.2f Gold x%.2f horn=%s on %s"),
		EliteSeed, RolledScale, RolledColour.R, RolledColour.G, RolledColour.B,
		RolledHealthMult, RolledSpeedMult, RolledGoldMult,
		bAddHorn ? TEXT("yes") : TEXT("no"), *Owner->GetName());
}
