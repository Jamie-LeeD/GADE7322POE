#include "UpgradeSparkBurstActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AUpgradeSparkBurstActor::AUpgradeSparkBurstActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		SparkMesh = SphereMesh.Object;
	}
}

void AUpgradeSparkBurstActor::SetSparkColor(FLinearColor InColor)
{
	SparkColor = InColor;
	SparkColor.R = FMath::Max(SparkColor.R, 0.35f) * 2.5f;
	SparkColor.G = FMath::Max(SparkColor.G, 0.35f) * 2.5f;
	SparkColor.B = FMath::Max(SparkColor.B, 0.15f) * 2.5f;
}

void AUpgradeSparkBurstActor::BeginPlay()
{
	Super::BeginPlay();
	if (!SparkMaterial)
	{
		SparkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_ProjectileSpark.M_ProjectileSpark"));
		if (!SparkMaterial)
		{
			SparkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
	}
	if (!bPlaying)
	{
		PlayBurst();
	}
}

void AUpgradeSparkBurstActor::PlayBurst(int32 SparkCount)
{
	Sparks.Reset();
	Age = 0.f;
	bPlaying = true;

	if (!SparkMesh)
	{
		return;
	}
	if (!SparkMaterial)
	{
		SparkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_ProjectileSpark.M_ProjectileSpark"));
		if (!SparkMaterial)
		{
			SparkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
	}

	UMaterialInterface* UseMat = SparkMaterial;
	if (SparkMaterial)
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(SparkMaterial, this))
		{
			Mid->SetVectorParameterValue(TEXT("SparkColor"), SparkColor);
			Mid->SetVectorParameterValue(TEXT("Color"), SparkColor);
			UseMat = Mid;
		}
	}

	UWorld* World = GetWorld();
	const FVector Origin = GetActorLocation();
	for (int32 i = 0; i < SparkCount; ++i)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (!Comp)
		{
			continue;
		}
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(SparkMesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetAbsolute(true, true, true);
		if (World)
		{
			Comp->RegisterComponentWithWorld(World);
		}
		else
		{
			Comp->RegisterComponent();
		}
		if (UseMat)
		{
			Comp->SetMaterial(0, UseMat);
		}

		const float Yaw = FMath::FRandRange(0.f, 360.f);
		const float Pitch = FMath::FRandRange(35.f, 88.f);
		const FVector Dir = FRotator(Pitch, Yaw, 0.f).Vector();
		const float Speed = FMath::FRandRange(220.f, 650.f);
		const float Scale = FMath::FRandRange(0.2f, 0.4f);
		Comp->SetWorldLocation(Origin);
		Comp->SetWorldScale3D(FVector(Scale));

		FSpark Spark;
		Spark.Mesh = Comp;
		Spark.Velocity = Dir * Speed;
		Spark.MaxLife = FMath::FRandRange(0.3f, 0.6f);
		Spark.Life = Spark.MaxLife;
		Spark.StartScale = Scale;
		Sparks.Add(Spark);
	}
}

void AUpgradeSparkBurstActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bPlaying)
	{
		return;
	}

	Age += DeltaTime;
	bool bAnyAlive = false;
	for (FSpark& Spark : Sparks)
	{
		UStaticMeshComponent* Comp = Spark.Mesh.Get();
		if (!Comp)
		{
			continue;
		}
		Spark.Life -= DeltaTime;
		if (Spark.Life <= 0.f)
		{
			Comp->DestroyComponent();
			Spark.Mesh = nullptr;
			continue;
		}

		bAnyAlive = true;
		Spark.Velocity += FVector(0.f, 0.f, -420.f) * DeltaTime;
		Comp->AddWorldOffset(Spark.Velocity * DeltaTime);
		const float Alpha = FMath::Clamp(Spark.Life / Spark.MaxLife, 0.f, 1.f);
		Comp->SetWorldScale3D(FVector(Spark.StartScale * Alpha));
	}

	if (!bAnyAlive || Age > 1.2f)
	{
		Destroy();
	}
}
