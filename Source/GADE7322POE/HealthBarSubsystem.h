#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HealthBarSubsystem.generated.h"

class UHealthBarComponent;


UCLASS()
class GADE7322POE_API UHealthBarSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickableInEditor() const override { return false; }

protected:
	bool IsGameplayWorld() const;
	void ResolveClasses();
	void EnsureBarsForClass(UClass* ActorClass, const FVector& Offset);

	UPROPERTY()
	TObjectPtr<UClass> TowerClass;

	UPROPERTY()
	TObjectPtr<UClass> ArcherClass;

	UPROPERTY()
	TObjectPtr<UClass> GoblinClass;

	bool bClassesResolved = false;
	float NextScanAt = 0.f;
	float ClassResolveRetryAt = 0.f;
};
