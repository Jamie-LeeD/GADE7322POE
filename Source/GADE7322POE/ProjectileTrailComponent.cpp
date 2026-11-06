#include "ProjectileTrailComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectileTrail, Log, All);

UProjectileTrailComponent::UProjectileTrailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bAutoActivate = true;
	SparkInterval = 0.02f;
	SparksPerEmit = 3;
}

void UProjectileTrailComponent::Configure(FLinearColor InTrailColor)
{
	// Boost brightness so unlit emissive reads clearly in-game.
	TrailColor = InTrailColor;
	TrailColor.R = FMath::Max(TrailColor.R, 0.35f) * 2.5f;
	TrailColor.G = FMath::Max(TrailColor.G, 0.35f) * 2.5f;
	TrailColor.B = FMath::Max(TrailColor.B, 0.15f) * 2.5f;
	bConfigured = true;
	if (SparkMID)
	{
		SparkMID->SetVectorParameterValue(TEXT("SparkColor"), TrailColor);
		SparkMID->SetVectorParameterValue(TEXT("Color"), TrailColor);
	}
}

void UProjectileTrailComponent::BeginPlay()
{
	Super::BeginPlay();

	SparkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_ProjectileSpark.M_ProjectileSpark"));
	if (!SparkMaterial)
	{
		SparkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	SparkMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	TrailNiagara = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_ProjectileTrail.NS_ProjectileTrail"));
	TrailCascade = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/VFX/PS_ProjectileTrail.PS_ProjectileTrail"));

	if (SparkMaterial)
	{
		SparkMID = UMaterialInstanceDynamic::Create(SparkMaterial, this);
		if (SparkMID)
		{
			SparkMID->SetVectorParameterValue(TEXT("SparkColor"), TrailColor);
			SparkMID->SetVectorParameterValue(TEXT("Color"), TrailColor);
		}
	}

	UE_LOG(LogProjectileTrail, Warning, TEXT("Trail begin on %s mesh=%s mat=%s"),
		*GetNameSafe(GetOwner()), SparkMesh ? TEXT("ok") : TEXT("MISSING"), SparkMaterial ? TEXT("ok") : TEXT("MISSING"));

	EnsureAssetFx();
}

void UProjectileTrailComponent::EnsureAssetFx()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}
	USceneComponent* Attach = Owner->GetRootComponent();
	if (!Attach)
	{
		return;
	}

	if (TrailCascade && !CascadeComp)
	{
		CascadeComp = UGameplayStatics::SpawnEmitterAttached(
			TrailCascade,
			Attach,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true,
			EPSCPoolMethod::None,
			true);
	}
	else if (TrailNiagara && !NiagaraComp)
	{
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailNiagara,
			Attach,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true,
			true,
			ENCPoolMethod::None,
			true);
	}
}

void UProjectileTrailComponent::EmitMeshSparks(int32 Count)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !World || !SparkMesh)
	{
		return;
	}

	const FVector Origin = Owner->GetActorLocation();
	const FVector Backward = -Owner->GetActorForwardVector();

	for (int32 i = 0; i < Count; ++i)
	{
		// World-space sparks (not attached) so they leave a visible trail behind the shot.
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		if (!Comp)
		{
			continue;
		}
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(SparkMesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetGenerateOverlapEvents(false);
		Comp->bReceivesDecals = false;
		Comp->SetAbsolute(true, true, true);
		Comp->RegisterComponentWithWorld(World);
		if (SparkMID)
		{
			Comp->SetMaterial(0, SparkMID);
		}
		else if (SparkMaterial)
		{
			Comp->SetMaterial(0, SparkMaterial);
		}

		const FVector Jitter(
			FMath::FRandRange(-22.f, 22.f),
			FMath::FRandRange(-22.f, 22.f),
			FMath::FRandRange(-16.f, 16.f));
		Comp->SetWorldLocation(Origin + Backward * FMath::FRandRange(10.f, 40.f) + Jitter);
		// Sphere mesh is 100uu; 0.18–0.35 ≈ 18–35uu — readable from the TD camera.
		const float Scale = FMath::FRandRange(0.18f, 0.35f);
		Comp->SetWorldScale3D(FVector(Scale));

		FTrailSpark Spark;
		Spark.Mesh = Comp;
		Spark.Velocity = Backward * FMath::FRandRange(60.f, 160.f) + Jitter * 1.5f;
		Spark.MaxLife = FMath::FRandRange(0.2f, 0.4f);
		Spark.Life = Spark.MaxLife;
		Spark.StartScale = Scale;
		Sparks.Add(Spark);
	}
}

void UProjectileTrailComponent::TickMeshSparks(float DeltaTime)
{
	for (int32 i = Sparks.Num() - 1; i >= 0; --i)
	{
		FTrailSpark& Spark = Sparks[i];
		UStaticMeshComponent* Comp = Spark.Mesh.Get();
		if (!Comp)
		{
			Sparks.RemoveAtSwap(i);
			continue;
		}
		Spark.Life -= DeltaTime;
		if (Spark.Life <= 0.f)
		{
			Comp->DestroyComponent();
			Sparks.RemoveAtSwap(i);
			continue;
		}
		Comp->AddWorldOffset(Spark.Velocity * DeltaTime);
		const float Alpha = FMath::Clamp(Spark.Life / Spark.MaxLife, 0.f, 1.f);
		Comp->SetWorldScale3D(FVector(Spark.StartScale * Alpha));
	}
}

void UProjectileTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	EmitCooldown -= DeltaTime;
	if (EmitCooldown <= 0.f)
	{
		EmitCooldown = SparkInterval;
		EmitMeshSparks(SparksPerEmit);
	}
	TickMeshSparks(DeltaTime);
}

void UProjectileTrailComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (FTrailSpark& Spark : Sparks)
	{
		if (UStaticMeshComponent* Comp = Spark.Mesh.Get())
		{
			Comp->DestroyComponent();
		}
	}
	Sparks.Reset();
	Super::EndPlay(EndPlayReason);
}
