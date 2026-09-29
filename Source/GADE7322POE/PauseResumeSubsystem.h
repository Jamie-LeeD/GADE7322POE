#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PauseResumeSubsystem.generated.h"


UCLASS()
class GADE7322POE_API UPauseResumeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }

protected:
	bool IsGameplayWorld() const;
	void ResolveClasses();
	AActor* FindGameManager() const;
	bool IsPauseMenuVisible() const;
	uint8 GetGameStateByte(AActor* GameManager) const;
	void ForceUnpause(const TCHAR* Reason);

	UPROPERTY()
	TObjectPtr<UClass> GameManagerClass;

	UPROPERTY()
	TSubclassOf<UUserWidget> PauseMenuClass;

	bool bClassesResolved = false;
	bool bPauseMenuWasVisible = false;
	float ClassResolveRetryAt = 0.f;
};
