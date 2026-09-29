#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatDirectorSubsystem.generated.h"

/**
 * Drives tower / archer / goblin combat from C++ so gameplay does not depend on
 * broken Blueprint CombatTick timers (Set Timer by Function Name + Custom Event).
 */
UCLASS()
class GADE7322POE_API UCombatDirectorSubsystem : public UTickableWorldSubsystem
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
	bool IsGamePlaying() const;

	void ResolveClasses();
	void TickRangedAttackers(float Now);
	void TickGoblinMelee(float Now);
	void TickProjectiles(float DeltaTime);

	AActor* FindClosestAliveInRange(AActor* Self, UClass* TargetClass, float Range) const;
	bool IsActorAlive(AActor* Actor) const;
	float GetFloatProp(AActor* Actor, FName Name, float DefaultValue) const;
	bool GetBoolProp(AActor* Actor, FName Name, bool DefaultValue) const;
	UClass* GetClassProp(AActor* Actor, FName Name) const;
	FVector GetMuzzleLocation(AActor* Attacker) const;

	void FireProjectile(AActor* Attacker, AActor* Target, float Damage, UClass* ProjectileClass);
	void ApplyDamage(AActor* Target, float Damage, AActor* Causer);
	void AwardGoldForKill(AActor* Victim);
	AActor* FindGameManager() const;

	struct FActiveShot
	{
		TWeakObjectPtr<AActor> Visual;
		TWeakObjectPtr<AActor> Target;
		TWeakObjectPtr<AActor> Causer;
		float Damage = 0.f;
		float Speed = 1800.f;
		float HitRadius = 80.f;
		FVector Location = FVector::ZeroVector;
		float Age = 0.f;
		bool bDamageApplied = false;
		/** On-hit slow captured at fire time (0 = none), so it still lands if the shooter dies mid-flight. */
		float SlowPercent = 0.f;
		float SlowDuration = 0.f;
	};

	void ApplyShotHit(FActiveShot& Shot, AActor* Target, AActor* Causer);

	UPROPERTY()
	TObjectPtr<UClass> TowerClass;

	UPROPERTY()
	TObjectPtr<UClass> ArcherClass;

	UPROPERTY()
	TObjectPtr<UClass> GoblinClass;

	UPROPERTY()
	TObjectPtr<UClass> ProjectileClassFallback;

	UPROPERTY()
	TObjectPtr<UClass> GameManagerClass;

	TMap<TWeakObjectPtr<AActor>, float> NextAttackTime;
	TArray<FActiveShot> ActiveShots;

	bool bClassesResolved = false;
	float ClassResolveRetryAt = 0.f;
};
