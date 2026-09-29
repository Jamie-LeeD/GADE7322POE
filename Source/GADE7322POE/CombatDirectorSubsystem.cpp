#include "CombatDirectorSubsystem.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "EnemyStatusComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FrostAttackComponent.h"
#include "GoblinShamanComponent.h"
#include "RoyalGuardianComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatDirector, Log, All);

namespace CombatDirectorPrivate
{
	static void SetObjectProp(UObject* Obj, FName Name, UObject* Value)
	{
		if (!Obj)
		{
			return;
		}
		if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(Obj->GetClass(), Name))
		{
			Prop->SetObjectPropertyValue_InContainer(Obj, Value);
		}
	}

	static float GetNumeric(UObject* Obj, FName Name, float DefaultValue)
	{
		if (!Obj)
		{
			return DefaultValue;
		}
		if (FDoubleProperty* DoubleProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			return static_cast<float>(DoubleProp->GetPropertyValue_InContainer(Obj));
		}
		if (FFloatProperty* FloatProp = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			return FloatProp->GetPropertyValue_InContainer(Obj);
		}
		if (FIntProperty* IntProp = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
		{
			return static_cast<float>(IntProp->GetPropertyValue_InContainer(Obj));
		}
		return DefaultValue;
	}

	static void SetNumeric(UObject* Obj, FName Name, float Value)
	{
		if (!Obj)
		{
			return;
		}
		if (FDoubleProperty* DoubleProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			DoubleProp->SetPropertyValue_InContainer(Obj, static_cast<double>(Value));
			return;
		}
		if (FFloatProperty* FloatProp = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			FloatProp->SetPropertyValue_InContainer(Obj, Value);
			return;
		}
		if (FIntProperty* IntProp = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
		{
			IntProp->SetPropertyValue_InContainer(Obj, FMath::RoundToInt(Value));
		}
	}

	static void SetBoolProp(UObject* Obj, FName Name, bool Value)
	{
		if (!Obj)
		{
			return;
		}
		if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
		{
			Prop->SetPropertyValue_InContainer(Obj, Value);
		}
	}

	static UActorComponent* FindHealthComponent(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		auto HasCurrentHealth = [](UActorComponent* Comp) -> bool
		{
			return Comp && (
				FindFProperty<FDoubleProperty>(Comp->GetClass(), FName(TEXT("CurrentHealth"))) != nullptr ||
				FindFProperty<FFloatProperty>(Comp->GetClass(), FName(TEXT("CurrentHealth"))) != nullptr);
		};

		// 1) Prefer real health component class / exact name (avoid HealthText TextRender matches).
		for (UActorComponent* Comp : Components)
		{
			if (!Comp)
			{
				continue;
			}
			const FString Name = Comp->GetName();
			const FString ClassName = Comp->GetClass()->GetName();
			if (ClassName.Contains(TEXT("HealthComponent")) || Name.Equals(TEXT("Health")))
			{
				if (HasCurrentHealth(Comp))
				{
					return Comp;
				}
			}
		}

		// 2) Any component that actually stores CurrentHealth.
		for (UActorComponent* Comp : Components)
		{
			if (HasCurrentHealth(Comp))
			{
				return Comp;
			}
		}

		// 3) Object property named Health on the actor.
		for (TFieldIterator<FObjectProperty> It(Actor->GetClass()); It; ++It)
		{
			FObjectProperty* Prop = *It;
			if (!Prop->GetName().Equals(TEXT("Health")))
			{
				continue;
			}
			if (UActorComponent* Comp = Cast<UActorComponent>(Prop->GetObjectPropertyValue_InContainer(Actor)))
			{
				if (HasCurrentHealth(Comp))
				{
					return Comp;
				}
			}
		}
		return nullptr;
	}

	static void CallNoArg(UObject* Obj, FName FunctionName)
	{
		if (!Obj)
		{
			return;
		}
		if (UFunction* Fn = Obj->FindFunction(FunctionName))
		{
			if (Fn->ParmsSize == 0)
			{
				Obj->ProcessEvent(Fn, nullptr);
			}
		}
	}
}

void UCombatDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	NextAttackTime.Reset();
	ActiveShots.Reset();
	bClassesResolved = false;
	ClassResolveRetryAt = 0.f;
}

void UCombatDirectorSubsystem::Deinitialize()
{
	NextAttackTime.Reset();
	ActiveShots.Reset();
	TowerClass = nullptr;
	ArcherClass = nullptr;
	GoblinClass = nullptr;
	ProjectileClassFallback = nullptr;
	GameManagerClass = nullptr;
	Super::Deinitialize();
}

TStatId UCombatDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCombatDirectorSubsystem, STATGROUP_Tickables);
}

bool UCombatDirectorSubsystem::IsGameplayWorld() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const EWorldType::Type Type = World->WorldType;
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

void UCombatDirectorSubsystem::ResolveClasses()
{
	if (bClassesResolved)
	{
		return;
	}

	TowerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Tower/BP_PrincessTower.BP_PrincessTower_C"));
	ArcherClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_RoyalArcher.BP_RoyalArcher_C"));
	GoblinClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_GoblinEnemy.BP_GoblinEnemy_C"));
	ProjectileClassFallback = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Combat/BP_Projectile.BP_Projectile_C"));
	GameManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));

	bClassesResolved = TowerClass && ArcherClass && GoblinClass && ProjectileClassFallback;
	if (bClassesResolved)
	{
		UE_LOG(LogCombatDirector, Warning, TEXT("CombatDirector active (C++ movement + direct health)."));
	}
}

bool UCombatDirectorSubsystem::IsGamePlaying() const
{
	if (!GameManagerClass)
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(World, GameManagerClass, Managers);
	if (Managers.Num() == 0 || !IsValid(Managers[0]))
	{
		return true;
	}

	AActor* GM = Managers[0];
	if (UFunction* IsPlayingFn = GM->FindFunction(FName(TEXT("IsPlaying"))))
	{
		struct FIsPlayingParms
		{
			bool ReturnValue = false;
		} Parms;
		GM->ProcessEvent(IsPlayingFn, &Parms);
		return Parms.ReturnValue;
	}

	return !GetBoolProp(GM, FName(TEXT("bPaused")), false);
}

float UCombatDirectorSubsystem::GetFloatProp(AActor* Actor, FName Name, float DefaultValue) const
{
	return CombatDirectorPrivate::GetNumeric(Actor, Name, DefaultValue);
}

bool UCombatDirectorSubsystem::GetBoolProp(AActor* Actor, FName Name, bool DefaultValue) const
{
	if (!Actor)
	{
		return DefaultValue;
	}
	if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(Actor->GetClass(), Name))
	{
		return BoolProp->GetPropertyValue_InContainer(Actor);
	}
	return DefaultValue;
}

UClass* UCombatDirectorSubsystem::GetClassProp(AActor* Actor, FName Name) const
{
	if (!Actor)
	{
		return nullptr;
	}
	if (FClassProperty* ClassProp = FindFProperty<FClassProperty>(Actor->GetClass(), Name))
	{
		return Cast<UClass>(ClassProp->GetObjectPropertyValue_InContainer(Actor));
	}
	if (FSoftClassProperty* SoftClassProp = FindFProperty<FSoftClassProperty>(Actor->GetClass(), Name))
	{
		return Cast<UClass>(SoftClassProp->GetPropertyValue_InContainer(Actor).Get());
	}
	return nullptr;
}

bool UCombatDirectorSubsystem::IsActorAlive(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}
	if (GetBoolProp(Actor, FName(TEXT("bIsDead")), false))
	{
		return false;
	}

	if (UActorComponent* Health = CombatDirectorPrivate::FindHealthComponent(Actor))
	{
		const float Current = CombatDirectorPrivate::GetNumeric(Health, FName(TEXT("CurrentHealth")), -1.f);
		if (Current >= 0.f)
		{
			return Current > 0.f;
		}
	}

	return true;
}

AActor* UCombatDirectorSubsystem::FindClosestAliveInRange(AActor* Self, UClass* TargetClass, float Range) const
{
	if (!Self || !TargetClass || Range <= 0.f)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector Origin = Self->GetActorLocation();
	AActor* Best = nullptr;
	float BestDistSq = Range * Range;

	for (TActorIterator<AActor> It(World, TargetClass); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsActorAlive(Candidate) || Candidate == Self)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	return Best;
}

FVector UCombatDirectorSubsystem::GetMuzzleLocation(AActor* Attacker) const
{
	const FVector Loc = Attacker->GetActorLocation();
	if (FStructProperty* StructProp = FindFProperty<FStructProperty>(Attacker->GetClass(), FName(TEXT("MuzzleOffset"))))
	{
		if (StructProp->Struct == TBaseStructure<FVector>::Get())
		{
			if (const FVector* Offset = StructProp->ContainerPtrToValuePtr<FVector>(Attacker))
			{
				return Loc + *Offset;
			}
		}
	}
	return Loc + FVector(0.f, 0.f, 120.f);
}

void UCombatDirectorSubsystem::ApplyDamage(AActor* Target, float Damage, AActor* Causer)
{
	if (!IsValid(Target) || Damage <= 0.f)
	{
		return;
	}

	UActorComponent* Health = CombatDirectorPrivate::FindHealthComponent(Target);
	if (!Health)
	{
		UE_LOG(LogCombatDirector, Error, TEXT("No health component on %s (comps may be misnamed)"), *GetNameSafe(Target));
		return;
	}

	// Defensive abilities (Royal Guardian Shield Guard) reduce damage before it reaches the health component.
	if (const URoyalGuardianComponent* Guardian = Target->FindComponentByClass<URoyalGuardianComponent>())
	{
		Damage *= Guardian->GetIncomingDamageMultiplier();
	}

	const float MaxHealth = CombatDirectorPrivate::GetNumeric(Health, FName(TEXT("MaxHealth")), 100.f);
	float Current = CombatDirectorPrivate::GetNumeric(Health, FName(TEXT("CurrentHealth")), MaxHealth);
	const float Previous = Current;
	Current = FMath::Max(0.f, Current - Damage);
	CombatDirectorPrivate::SetNumeric(Health, FName(TEXT("CurrentHealth")), Current);

	// Verify write stuck (catches wrong-component bugs).
	const float Verify = CombatDirectorPrivate::GetNumeric(Health, FName(TEXT("CurrentHealth")), -1.f);
	UE_LOG(LogCombatDirector, Warning, TEXT("DAMAGE %s via %s: %.0f -> %.0f (verify=%.0f, by %s)"),
		*GetNameSafe(Target), *Health->GetName(), Previous, Current, Verify, *GetNameSafe(Causer));

	CombatDirectorPrivate::SetNumeric(Target, FName(TEXT("CurrentHealth")), Current);
	if (Current <= 0.f)
	{
		CombatDirectorPrivate::SetBoolProp(Target, FName(TEXT("bIsDead")), true);
		CombatDirectorPrivate::SetBoolProp(Health, FName(TEXT("bIsDead")), true);
	}

	// Drive BP display: UpdateHealthDisplay + push text onto HealthText if present.
	CombatDirectorPrivate::CallNoArg(Health, FName(TEXT("UpdateHealthDisplay")));
	CombatDirectorPrivate::CallNoArg(Target, FName(TEXT("UpdateHealthDisplay")));

	if (UFunction* ChangedFn = Target->FindFunction(FName(TEXT("OnGoblinHealthChanged"))))
	{
		struct FHealthChangedParms
		{
			double Current = 0.0;
			double Max = 0.0;
			double Percent = 0.0;
		} Parms;
		Parms.Current = Current;
		Parms.Max = MaxHealth;
		Parms.Percent = (MaxHealth > 0.f) ? (Current / MaxHealth) : 0.f;
		if (ChangedFn->ParmsSize <= sizeof(Parms))
		{
			Target->ProcessEvent(ChangedFn, &Parms);
		}
	}
	if (UFunction* ChangedFn = Target->FindFunction(FName(TEXT("OnTowerHealthChanged"))))
	{
		struct FHealthChangedParms
		{
			double Current = 0.0;
			double Max = 0.0;
			double Percent = 0.0;
		} Parms;
		Parms.Current = Current;
		Parms.Max = MaxHealth;
		Parms.Percent = (MaxHealth > 0.f) ? (Current / MaxHealth) : 0.f;
		if (ChangedFn->ParmsSize <= sizeof(Parms))
		{
			Target->ProcessEvent(ChangedFn, &Parms);
		}
	}

	// Directly set floating HealthText if the BP event path fails.
	TArray<UActorComponent*> Components;
	Target->GetComponents(Components);
	for (UActorComponent* Comp : Components)
	{
		if (!Comp || !Comp->GetName().Contains(TEXT("HealthText")))
		{
			continue;
		}
		if (UFunction* SetTextFn = Comp->FindFunction(FName(TEXT("SetText"))))
		{
			struct FSetTextParms
			{
				FText Value;
			} Parms;
			Parms.Value = FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Current, MaxHealth));
			Comp->ProcessEvent(SetTextFn, &Parms);
		}
		else if (UFunction* K2SetTextFn = Comp->FindFunction(FName(TEXT("K2_SetText"))))
		{
			struct FSetTextParms
			{
				FText Value;
			} Parms;
			Parms.Value = FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Current, MaxHealth));
			Comp->ProcessEvent(K2SetTextFn, &Parms);
		}
	}

	if (Current <= 0.f && Previous > 0.f)
	{
		// Pay gold before BP death handlers / Destroy (BP AddGold path is unreliable).
		AwardGoldForKill(Target);

		CombatDirectorPrivate::CallNoArg(Health, FName(TEXT("OnDeath")));
		CombatDirectorPrivate::CallNoArg(Target, FName(TEXT("HandleGoblinDeath")));
		CombatDirectorPrivate::CallNoArg(Target, FName(TEXT("HandleArcherDeath")));
		CombatDirectorPrivate::CallNoArg(Target, FName(TEXT("HandleTowerDeath")));
		CombatDirectorPrivate::CallNoArg(Target, FName(TEXT("OnDeath")));
		CombatDirectorPrivate::CallNoArg(Target, FName(TEXT("NotifySpawnerOfDeath")));
		CombatDirectorPrivate::CallNoArg(Target, FName(TEXT("NotifyEnemyDied")));

		if (IsValid(Target))
		{
			Target->SetActorHiddenInGame(true);
			Target->SetActorEnableCollision(false);
			Target->Destroy();
		}
	}
}

AActor* UCombatDirectorSubsystem::FindGameManager() const
{
	if (!GameManagerClass)
	{
		return nullptr;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(World, GameManagerClass, Managers);
	return (Managers.Num() > 0) ? Managers[0] : nullptr;
}

void UCombatDirectorSubsystem::AwardGoldForKill(AActor* Victim)
{
	if (!IsValid(Victim) || !GoblinClass || !Victim->IsA(GoblinClass))
	{
		return;
	}

	AActor* GM = FindGameManager();
	if (!IsValid(GM))
	{
		UE_LOG(LogCombatDirector, Warning, TEXT("AwardGold: no GameManager found"));
		return;
	}

	const float Reward = FMath::Max(0.f, GetFloatProp(Victim, FName(TEXT("GoldReward")), 25.f));
	if (Reward <= 0.f)
	{
		return;
	}

	const float OldGold = CombatDirectorPrivate::GetNumeric(GM, FName(TEXT("Gold")), 0.f);

	// Prefer Blueprint AddGold(Amount).
	if (UFunction* AddGoldFn = GM->FindFunction(FName(TEXT("AddGold"))))
	{
		TArray<uint8> Parms;
		Parms.SetNumZeroed(AddGoldFn->ParmsSize);

		for (TFieldIterator<FProperty> It(AddGoldFn); It && (It->PropertyFlags & CPF_Parm); ++It)
		{
			FProperty* Prop = *It;
			if (Prop->PropertyFlags & CPF_ReturnParm)
			{
				continue;
			}

			if (FDoubleProperty* AsDouble = CastField<FDoubleProperty>(Prop))
			{
				AsDouble->SetPropertyValue_InContainer(Parms.GetData(), static_cast<double>(Reward));
			}
			else if (FFloatProperty* AsFloat = CastField<FFloatProperty>(Prop))
			{
				AsFloat->SetPropertyValue_InContainer(Parms.GetData(), Reward);
			}
			else if (FIntProperty* AsInt = CastField<FIntProperty>(Prop))
			{
				AsInt->SetPropertyValue_InContainer(Parms.GetData(), FMath::RoundToInt(Reward));
			}
			else if (FInt64Property* AsInt64 = CastField<FInt64Property>(Prop))
			{
				AsInt64->SetPropertyValue_InContainer(Parms.GetData(), static_cast<int64>(FMath::RoundToInt(Reward)));
			}
		}

		GM->ProcessEvent(AddGoldFn, Parms.GetData());
	}

	float NewGold = CombatDirectorPrivate::GetNumeric(GM, FName(TEXT("Gold")), OldGold);
	if (NewGold <= OldGold)
	{
		// AddGold didn't stick â€” apply directly.
		NewGold = OldGold + Reward;
		CombatDirectorPrivate::SetNumeric(GM, FName(TEXT("Gold")), NewGold);
	}

	if (UFunction* GoldChangedFn = GM->FindFunction(FName(TEXT("OnGoldChanged"))))
	{
		struct FGoldChangedParms
		{
			double NewGold = 0.0;
		} Parms;
		Parms.NewGold = NewGold;
		if (GoldChangedFn->ParmsSize <= sizeof(Parms))
		{
			GM->ProcessEvent(GoldChangedFn, &Parms);
		}
		else if (GoldChangedFn->ParmsSize == 0)
		{
			GM->ProcessEvent(GoldChangedFn, nullptr);
		}
	}
	CombatDirectorPrivate::CallNoArg(GM, FName(TEXT("UpdateGoldDisplay")));
	CombatDirectorPrivate::CallNoArg(GM, FName(TEXT("RefreshHUD")));

	UE_LOG(LogCombatDirector, Warning, TEXT("GOLD +%.0f for killing %s | GameManager Gold %.0f -> %.0f"),
		Reward, *GetNameSafe(Victim), OldGold, NewGold);
}

void UCombatDirectorSubsystem::FireProjectile(AActor* Attacker, AActor* Target, float Damage, UClass* ProjectileClass)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Attacker) || !IsValid(Target) || !ProjectileClass)
	{
		return;
	}

	const FVector Muzzle = GetMuzzleLocation(Attacker);
	const FVector ToTarget = Target->GetActorLocation() - Muzzle;
	const FRotator Rotation = ToTarget.IsNearlyZero() ? Attacker->GetActorRotation() : ToTarget.GetSafeNormal().Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Attacker;
	SpawnParams.Instigator = Cast<APawn>(Attacker);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const UFrostAttackComponent* Frost = Attacker->FindComponentByClass<UFrostAttackComponent>();
	const UGoblinShamanComponent* Shaman = Attacker->FindComponentByClass<UGoblinShamanComponent>();

	AActor* Projectile = World->SpawnActor<AActor>(ProjectileClass, Muzzle, Rotation, SpawnParams);
	if (IsValid(Projectile))
	{
		// Blueprint Tick/MoveTowardTarget was NaN-corrupting shots before they could hit goblins.
		Projectile->SetActorTickEnabled(false);
		Projectile->PrimaryActorTick.bCanEverTick = false;
		Projectile->SetActorScale3D(FVector(2.5f));
		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
		{
			RootPrim->SetVisibility(true, true);
			RootPrim->SetHiddenInGame(false, true);
		}
		if (Frost)
		{
			Frost->TintProjectile(Projectile);
		}
		if (Shaman)
		{
			Shaman->TintProjectile(Projectile);
		}
	}
	else
	{
		ApplyDamage(Target, Damage, Attacker);
		if (Frost && IsActorAlive(Target))
		{
			Frost->ApplyFrostOnHit(Target);
		}
		UE_LOG(LogCombatDirector, Warning, TEXT("Projectile spawn failed; applied direct damage to %s"), *GetNameSafe(Target));
		return;
	}

	FActiveShot Shot;
	Shot.Visual = Projectile;
	Shot.Target = Target;
	Shot.Causer = Attacker;
	Shot.Damage = Damage;
	Shot.Speed = 1800.f;
	Shot.HitRadius = 100.f;
	Shot.Location = Muzzle;
	Shot.Age = 0.f;
	Shot.bDamageApplied = false;
	if (Frost)
	{
		Shot.SlowPercent = Frost->SlowPercent;
		Shot.SlowDuration = Frost->SlowDuration;
	}
	ActiveShots.Add(Shot);

	DrawDebugLine(World, Muzzle, Target->GetActorLocation(), FColor::Yellow, false, 0.2f, 0, 4.f);
	UE_LOG(LogCombatDirector, Warning, TEXT("%s shot %s (%.0f dmg) [tracked]"), *GetNameSafe(Attacker), *GetNameSafe(Target), Damage);
}

void UCombatDirectorSubsystem::TickRangedAttackers(float Now)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	auto ProcessAttacker = [&](UClass* AttackerClass)
	{
		if (!AttackerClass)
		{
			return;
		}

		for (TActorIterator<AActor> It(World, AttackerClass); It; ++It)
		{
			AActor* Attacker = *It;
			if (!IsActorAlive(Attacker))
			{
				continue;
			}
			if (!GetBoolProp(Attacker, FName(TEXT("bEnableCombat")), true))
			{
				continue;
			}

			if (const float* ReadyAt = NextAttackTime.Find(Attacker))
			{
				if (Now < *ReadyAt)
				{
					continue;
				}
			}

			const float Range = GetFloatProp(Attacker, FName(TEXT("AttackRange")), 800.f);
			const float Damage = GetFloatProp(Attacker, FName(TEXT("AttackDamage")), 20.f);
			const float Cooldown = FMath::Max(0.15f, GetFloatProp(Attacker, FName(TEXT("AttackCooldown")), 1.f));

			AActor* Target = FindClosestAliveInRange(Attacker, GoblinClass, Range);
			if (!Target)
			{
				continue;
			}

			if (URoyalGuardianComponent* Guardian = Attacker->FindComponentByClass<URoyalGuardianComponent>(); Guardian && Guardian->bUseMeleeAttack)
			{
				Guardian->NotifyMeleeStrike(Target);
				DrawDebugLine(World, Attacker->GetActorLocation(), Target->GetActorLocation(), FColor::Orange, false, 0.2f, 0, 5.f);
				ApplyDamage(Target, Damage, Attacker);
				NextAttackTime.Add(Attacker, Now + Cooldown);
				continue;
			}

			UClass* ProjClass = GetClassProp(Attacker, FName(TEXT("ProjectileClass")));
			if (!ProjClass)
			{
				ProjClass = ProjectileClassFallback;
			}

			FireProjectile(Attacker, Target, Damage, ProjClass);
			NextAttackTime.Add(Attacker, Now + Cooldown);
		}
	};

	ProcessAttacker(TowerClass);
	ProcessAttacker(ArcherClass);
}

void UCombatDirectorSubsystem::TickGoblinMelee(float Now)
{
	UWorld* World = GetWorld();
	if (!World || !GoblinClass)
	{
		return;
	}

	for (TActorIterator<AActor> It(World, GoblinClass); It; ++It)
	{
		AActor* Goblin = *It;
		if (!IsActorAlive(Goblin))
		{
			continue;
		}
		if (!GetBoolProp(Goblin, FName(TEXT("bEnableMelee")), true))
		{
			continue;
		}

		if (const float* ReadyAt = NextAttackTime.Find(Goblin))
		{
			if (Now < *ReadyAt)
			{
				continue;
			}
		}

		const float Range = GetFloatProp(Goblin, FName(TEXT("AttackRange")), 280.f);
		float Damage = GetFloatProp(Goblin, FName(TEXT("AttackDamage")), 10.f);
		if (const UEnemyStatusComponent* Status = Goblin->FindComponentByClass<UEnemyStatusComponent>())
		{
			Damage *= Status->GetDamageMultiplier();
		}
		const float Cooldown = FMath::Max(0.15f, GetFloatProp(Goblin, FName(TEXT("AttackCooldown")), 1.f));

		AActor* Target = FindClosestAliveInRange(Goblin, ArcherClass, Range);
		if (!Target)
		{
			Target = FindClosestAliveInRange(Goblin, TowerClass, Range);
		}
		if (!Target)
		{
			continue;
		}

		if (const UGoblinShamanComponent* Shaman = Goblin->FindComponentByClass<UGoblinShamanComponent>(); Shaman && Shaman->bUseMagicBolt)
		{
			FireProjectile(Goblin, Target, Damage, ProjectileClassFallback);
			NextAttackTime.Add(Goblin, Now + Cooldown);
			continue;
		}

		ApplyDamage(Target, Damage, Goblin);
		NextAttackTime.Add(Goblin, Now + Cooldown);

		DrawDebugLine(World, Goblin->GetActorLocation(), Target->GetActorLocation(), FColor::Red, false, 0.15f, 0, 3.f);
	}
}

void UCombatDirectorSubsystem::ApplyShotHit(FActiveShot& Shot, AActor* Target, AActor* Causer)
{
	if (Shot.bDamageApplied)
	{
		return;
	}
	Shot.bDamageApplied = true;

	ApplyDamage(Target, Shot.Damage, Causer);

	if (Shot.SlowPercent > 0.f && Shot.SlowDuration > 0.f && IsActorAlive(Target))
	{
		if (UEnemyStatusComponent* Status = UEnemyStatusComponent::FindOrAddTo(Target))
		{
			Status->ApplySlow(Shot.SlowPercent, Shot.SlowDuration);
		}
	}
}

void UCombatDirectorSubsystem::TickProjectiles(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 Index = ActiveShots.Num() - 1; Index >= 0; --Index)
	{
		FActiveShot& Shot = ActiveShots[Index];
		Shot.Age += DeltaTime;

		AActor* Target = Shot.Target.Get();
		AActor* Causer = Shot.Causer.Get();
		AActor* Visual = Shot.Visual.Get();

		const bool bTargetOk = IsValid(Target) && IsActorAlive(Target);
		if (!bTargetOk)
		{
			if (IsValid(Visual))
			{
				Visual->Destroy();
			}
			ActiveShots.RemoveAtSwap(Index);
			continue;
		}

		const FVector Goal = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		FVector Delta = Goal - Shot.Location;
		if (Delta.ContainsNaN() || Shot.Location.ContainsNaN())
		{
			ApplyShotHit(Shot, Target, Causer);
			if (IsValid(Visual))
			{
				Visual->Destroy();
			}
			ActiveShots.RemoveAtSwap(Index);
			continue;
		}

		const float Dist = static_cast<float>(Delta.Size());
		const bool bShouldHit = Dist <= Shot.HitRadius || Shot.Age >= 2.5f;
		if (bShouldHit)
		{
			ApplyShotHit(Shot, Target, Causer);
			if (IsValid(Visual))
			{
				Visual->Destroy();
			}
			ActiveShots.RemoveAtSwap(Index);
			continue;
		}

		Shot.Location += Delta.GetSafeNormal() * Shot.Speed * DeltaTime;
		if (IsValid(Visual))
		{
			Visual->SetActorLocation(Shot.Location);
			Visual->SetActorRotation(Delta.Rotation());
		}
		DrawDebugSphere(World, Shot.Location, 28.f, 6, FColor::Yellow, false, -1.f, 0, 1.5f);
	}
}

void UCombatDirectorSubsystem::Tick(float DeltaTime)
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

	if (!IsGamePlaying())
	{
		return;
	}

	TickRangedAttackers(Now);
	TickGoblinMelee(Now);
	TickProjectiles(DeltaTime);
}
