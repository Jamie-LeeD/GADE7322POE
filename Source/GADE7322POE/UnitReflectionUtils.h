#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"

/** Reflection helpers for reading/writing Blueprint variables (MoveSpeed, AttackDamage, CurrentHealth...). */
namespace UnitReflection
{
	inline bool HasNumeric(const UObject* Obj, FName Name)
	{
		return Obj && (FindFProperty<FDoubleProperty>(Obj->GetClass(), Name) || FindFProperty<FFloatProperty>(Obj->GetClass(), Name));
	}

	inline float GetNumeric(const UObject* Obj, FName Name, float DefaultValue)
	{
		if (!Obj)
		{
			return DefaultValue;
		}
		if (const FDoubleProperty* DoubleProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			return static_cast<float>(DoubleProp->GetPropertyValue_InContainer(Obj));
		}
		if (const FFloatProperty* FloatProp = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			return FloatProp->GetPropertyValue_InContainer(Obj);
		}
		if (const FIntProperty* IntProp = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
		{
			return static_cast<float>(IntProp->GetPropertyValue_InContainer(Obj));
		}
		return DefaultValue;
	}

	inline void SetNumeric(UObject* Obj, FName Name, float Value)
	{
		if (!Obj)
		{
			return;
		}
		if (FDoubleProperty* DoubleProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			DoubleProp->SetPropertyValue_InContainer(Obj, static_cast<double>(Value));
		}
		else if (FFloatProperty* FloatProp = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			FloatProp->SetPropertyValue_InContainer(Obj, Value);
		}
		else if (FIntProperty* IntProp = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
		{
			IntProp->SetPropertyValue_InContainer(Obj, FMath::RoundToInt(Value));
		}
	}

	/** Same lookup rules as the combat director: the BP_HealthComponent (never the HealthText TextRender). */
	inline UActorComponent* FindHealthComponent(const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		const FName CurrentHealthName(TEXT("CurrentHealth"));
		for (UActorComponent* Comp : Components)
		{
			if (Comp && HasNumeric(Comp, CurrentHealthName) &&
				(Comp->GetClass()->GetName().Contains(TEXT("HealthComponent")) || Comp->GetName().Equals(TEXT("Health"))))
			{
				return Comp;
			}
		}
		for (UActorComponent* Comp : Components)
		{
			if (Comp && HasNumeric(Comp, CurrentHealthName))
			{
				return Comp;
			}
		}
		return nullptr;
	}

	/** Applies a MaxHealth value to the owner's existing health component (full heal). */
	inline void ApplyMaxHealthOverride(const AActor* Actor, float MaxHealth)
	{
		if (MaxHealth <= 0.f)
		{
			return;
		}
		if (UActorComponent* Health = FindHealthComponent(Actor))
		{
			SetNumeric(Health, FName(TEXT("MaxHealth")), MaxHealth);
			SetNumeric(Health, FName(TEXT("CurrentHealth")), MaxHealth);
		}
	}

	inline UMaterialInterface* GetTintBaseMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Cached.Get();
	}

	/** Collects the owner's visible body meshes (static mesh components). */
	inline void GetBodyMeshes(const AActor* Actor, TArray<UStaticMeshComponent*>& OutMeshes)
	{
		OutMeshes.Reset();
		if (Actor)
		{
			Actor->GetComponents(OutMeshes);
		}
	}
}
