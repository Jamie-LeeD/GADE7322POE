#include "CartoonSkyboxActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkyLight.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"

ACartoonSkyboxActor::ACartoonSkyboxActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	SkyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyMesh"));
	SetRootComponent(SkyMesh);
	SkyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyMesh->SetCastShadow(false);
	SkyMesh->bReceivesDecals = false;
	SkyMesh->SetGenerateOverlapEvents(false);
	SkyMesh->bAffectDistanceFieldLighting = false;
	// Moving sky must not feed real-time SkyLight captures (that flickers lit tiles).
	SkyMesh->bVisibleInRealTimeSkyCaptures = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		SkyMesh->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SkyMI(
		TEXT("/Game/QMS_Cartoon_Skybox_Free/Material_Instances/MI_Sky0002_Day.MI_Sky0002_Day"));
	if (SkyMI.Succeeded())
	{
		SkyMesh->SetMaterial(0, SkyMI.Object);
	}

	SetActorScale3D(FVector(SphereScale));
}

void ACartoonSkyboxActor::StabilizeSkyLights() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		if (USkyLightComponent* Light = It->GetLightComponent())
		{
			Light->bRealTimeCapture = false;
			Light->bLowerHemisphereIsBlack = false;
			Light->LowerHemisphereColor = FLinearColor(0.55f, 0.7f, 0.9f, 1.f);
			Light->SetIntensity(3.f);
			Light->RecaptureSky();
		}
	}
}

void ACartoonSkyboxActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorScale3D(FVector(SphereScale));
	StabilizeSkyLights();
	SetActorTickEnabled(true);
}

void ACartoonSkyboxActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector ViewLoc = GetActorLocation();
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APlayerCameraManager* CamMgr = PC->PlayerCameraManager)
		{
			ViewLoc = CamMgr->GetCameraLocation();
		}
		else
		{
			FRotator ViewRot;
			PC->GetPlayerViewPoint(ViewLoc, ViewRot);
		}
	}

	SetActorLocation(ViewLoc, false, nullptr, ETeleportType::TeleportPhysics);
}
