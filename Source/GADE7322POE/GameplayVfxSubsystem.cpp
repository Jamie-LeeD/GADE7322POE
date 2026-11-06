#include "GameplayVfxSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "UnitReflectionUtils.h"
#include "ProjectileTrailComponent.h"
#include "UpgradeSparkBurstActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameplayVfx, Log, All);

void UGameplayVfxSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ThreatIntensity = 0.f;
	bAssetsResolved = false;
}

void UGameplayVfxSubsystem::Deinitialize()
{
	if (IsValid(ThreatVolume))
	{
		ThreatVolume->Destroy();
	}
	ThreatVolume = nullptr;
	ThreatMID = nullptr;
	ThreatPostProcessMaterial = nullptr;
	UpgradeBurstSystem = nullptr;
	UpgradeBurstCascade = nullptr;
	Super::Deinitialize();
}

TStatId UGameplayVfxSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGameplayVfxSubsystem, STATGROUP_Tickables);
}

bool UGameplayVfxSubsystem::IsGameplayWorld() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const EWorldType::Type Type = World->WorldType;
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

void UGameplayVfxSubsystem::EnsureAssets()
{
	if (bAssetsResolved)
	{
		return;
	}

	UpgradeBurstSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_UpgradeBurst.NS_UpgradeBurst"));
	UpgradeBurstCascade = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/VFX/PS_UpgradeBurst.PS_UpgradeBurst"));
	ThreatPostProcessMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_PP_TowerThreat.M_PP_TowerThreat"));
	TowerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Tower/BP_PrincessTower.BP_PrincessTower_C"));

	bAssetsResolved = (UpgradeBurstSystem != nullptr) || (UpgradeBurstCascade != nullptr) || (ThreatPostProcessMaterial != nullptr);
	if (UpgradeBurstCascade)
	{
		UE_LOG(LogGameplayVfx, Warning, TEXT("Loaded PS_UpgradeBurst (Cascade)"));
	}
	else if (UpgradeBurstSystem)
	{
		UE_LOG(LogGameplayVfx, Warning, TEXT("Loaded NS_UpgradeBurst (Niagara)"));
	}
	else
	{
		UE_LOG(LogGameplayVfx, Warning, TEXT("Upgrade burst particle missing — run Tools/CreatePart3VFX.py"));
	}
	if (ThreatPostProcessMaterial)
	{
		UE_LOG(LogGameplayVfx, Warning, TEXT("Loaded M_PP_TowerThreat"));
	}
}

void UGameplayVfxSubsystem::EnsurePostProcessVolume()
{
	UWorld* World = GetWorld();
	if (!World || !ThreatPostProcessMaterial)
	{
		return;
	}
	if (IsValid(ThreatVolume))
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ThreatVolume = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!ThreatVolume)
	{
		return;
	}

	ThreatVolume->bUnbound = true;
	ThreatVolume->Priority = 10.f;
	ThreatVolume->BlendWeight = 1.f;

	ThreatMID = UMaterialInstanceDynamic::Create(ThreatPostProcessMaterial, this);
	if (ThreatMID)
	{
		ThreatMID->SetScalarParameterValue(TEXT("ThreatIntensity"), 0.f);
		FWeightedBlendable Blendable;
		Blendable.Object = ThreatMID;
		Blendable.Weight = 1.f;
		ThreatVolume->Settings.WeightedBlendables.Array.Add(Blendable);
	}
	UE_LOG(LogGameplayVfx, Warning, TEXT("Spawned unbound PostProcessVolume for tower threat VFX"));
}

float UGameplayVfxSubsystem::GetTowerHealthPercent() const
{
	UWorld* World = GetWorld();
	if (!World || !TowerClass)
	{
		return 1.f;
	}
	for (TActorIterator<AActor> It(World, TowerClass); It; ++It)
	{
		AActor* Tower = *It;
		if (!IsValid(Tower))
		{
			continue;
		}
		if (UActorComponent* Health = UnitReflection::FindHealthComponent(Tower))
		{
			const float Max = UnitReflection::GetNumeric(Health, FName(TEXT("MaxHealth")), 0.f);
			const float Cur = UnitReflection::GetNumeric(Health, FName(TEXT("CurrentHealth")), Max);
			return Max > 0.f ? FMath::Clamp(Cur / Max, 0.f, 1.f) : 1.f;
		}
		const float Max = UnitReflection::GetNumeric(Tower, FName(TEXT("MaxHealth")), 0.f);
		const float Cur = UnitReflection::GetNumeric(Tower, FName(TEXT("CurrentHealth")), Max);
		if (Max > 0.f)
		{
			return FMath::Clamp(Cur / Max, 0.f, 1.f);
		}
	}
	return 1.f;
}

void UGameplayVfxSubsystem::UpdateTowerThreat(float DeltaTime)
{
	EnsurePostProcessVolume();
	if (!ThreatMID)
	{
		return;
	}

	const float Hp = GetTowerHealthPercent();
	// Threat ramps as HP falls below 70%, stronger under 40%.
	float Target = 0.f;
	if (Hp < 0.7f)
	{
		Target = FMath::GetMappedRangeValueClamped(FVector2D(0.7f, 0.15f), FVector2D(0.f, 1.f), Hp);
	}
	ThreatIntensity = FMath::FInterpTo(ThreatIntensity, Target, DeltaTime, 3.f);
	ThreatMID->SetScalarParameterValue(TEXT("ThreatIntensity"), ThreatIntensity);
	if (ThreatVolume)
	{
		ThreatVolume->BlendWeight = ThreatIntensity > 0.01f ? 1.f : 0.f;
	}
}

void UGameplayVfxSubsystem::PlayUpgradeBurstAt(FVector WorldLocation)
{
	EnsureAssets();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Authored graph particle assets (Cascade preferred, Niagara fallback).
	if (UpgradeBurstCascade)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			UpgradeBurstCascade,
			WorldLocation,
			FRotator::ZeroRotator,
			FVector(1.f),
			true,
			EPSCPoolMethod::AutoRelease,
			true);
	}
	else if (UpgradeBurstSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			UpgradeBurstSystem,
			WorldLocation,
			FRotator::ZeroRotator,
			FVector(1.f),
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true);
	}

	// Custom mesh-spark particle burst — always visible upgrade feedback.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AUpgradeSparkBurstActor* Burst = World->SpawnActorDeferred<AUpgradeSparkBurstActor>(
		AUpgradeSparkBurstActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, WorldLocation),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Burst->SetSparkColor(FLinearColor(1.f, 0.75f, 0.2f));
		Burst->PlayBurst(28);
		UGameplayStatics::FinishSpawningActor(Burst, FTransform(FRotator::ZeroRotator, WorldLocation));
	}
}void UGameplayVfxSubsystem::PlayUpgradeBurstForActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		PlayUpgradeBurstAt(Actor->GetActorLocation() + FVector(0.f, 0.f, 60.f));
	}
}

void UGameplayVfxSubsystem::AttachProjectileTrail(AActor* Projectile, FLinearColor TrailColor)
{
	if (!IsValid(Projectile))
	{
		return;
	}
	if (Projectile->FindComponentByClass<UProjectileTrailComponent>())
	{
		return;
	}

	UProjectileTrailComponent* Trail = NewObject<UProjectileTrailComponent>(Projectile, TEXT("ProjectileTrail"));
	if (!Trail)
	{
		return;
	}
	Trail->Configure(TrailColor);
	Projectile->AddInstanceComponent(Trail);
	Trail->RegisterComponent();
	UE_LOG(LogGameplayVfx, Warning, TEXT("Attached projectile trail to %s"), *GetNameSafe(Projectile));
}

void UGameplayVfxSubsystem::PlayProjectileImpactAt(FVector WorldLocation, FLinearColor ImpactColor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (AUpgradeSparkBurstActor* Burst = World->SpawnActorDeferred<AUpgradeSparkBurstActor>(
		AUpgradeSparkBurstActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, WorldLocation),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Burst->SetSparkColor(ImpactColor);
		Burst->PlayBurst(18);
		UGameplayStatics::FinishSpawningActor(Burst, FTransform(FRotator::ZeroRotator, WorldLocation));
	}
}

void UGameplayVfxSubsystem::Tick(float DeltaTime)
{
	if (!IsGameplayWorld())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (!bAssetsResolved)
	{
		if (Now < AssetRetryAt)
		{
			return;
		}
		EnsureAssets();
		AssetRetryAt = Now + 1.f;
	}

	UpdateTowerThreat(DeltaTime);
}
