#include "UI/HealthBarSubsystem.h"

#include "UI/HealthBarComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogHealthBar, Log, All);

void UHealthBarSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bClassesResolved = false;
	NextScanAt = 0.f;
	ClassResolveRetryAt = 0.f;
}

void UHealthBarSubsystem::Deinitialize()
{
	TowerClass = nullptr;
	ArcherClass = nullptr;
	GoblinClass = nullptr;
	Super::Deinitialize();
}

TStatId UHealthBarSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UHealthBarSubsystem, STATGROUP_Tickables);
}

bool UHealthBarSubsystem::IsGameplayWorld() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const EWorldType::Type Type = World->WorldType;
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

void UHealthBarSubsystem::ResolveClasses()
{
	if (bClassesResolved)
	{
		return;
	}

	TowerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Tower/BP_PrincessTower.BP_PrincessTower_C"));
	ArcherClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalArcher.BP_RoyalArcher_C"));
	GoblinClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinEnemy.BP_GoblinEnemy_C"));
	bClassesResolved = TowerClass && ArcherClass && GoblinClass;
	if (bClassesResolved)
	{
		UE_LOG(LogHealthBar, Warning, TEXT("HealthBarSubsystem active — auto-attaching reusable health bars."));
	}
}

void UHealthBarSubsystem::EnsureBarsForClass(UClass* ActorClass, const FVector& Offset)
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return;
	}

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->FindComponentByClass<UHealthBarComponent>())
		{
			continue;
		}

		UHealthBarComponent* Bar = NewObject<UHealthBarComponent>(Actor, UHealthBarComponent::StaticClass(), TEXT("AutoHealthBar"));
		Bar->WorldOffset = Offset;
		Bar->bHideOwnerHealthText = true;
		Bar->bShowNumericText = false;
		Bar->RegisterComponent();
		Actor->AddInstanceComponent(Bar);
		Bar->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Bar->SetRelativeLocation(Offset);
	}
}

void UHealthBarSubsystem::Tick(float DeltaTime)
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
	if (!bClassesResolved)
	{
		if (Now >= ClassResolveRetryAt)
		{
			ResolveClasses();
			ClassResolveRetryAt = Now + 1.f;
		}
		if (!bClassesResolved)
		{
			return;
		}
	}

	if (Now < NextScanAt)
	{
		return;
	}
	NextScanAt = Now + 0.5f;

	EnsureBarsForClass(TowerClass, FVector(0.f, 0.f, 320.f));
	EnsureBarsForClass(ArcherClass, FVector(0.f, 0.f, 160.f));
	EnsureBarsForClass(GoblinClass, FVector(0.f, 0.f, 120.f));
}
