#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

	/** Actor-local centre and half-size of the owner's body meshes (used to place attachments). */
	inline void GetBodyBounds(const AActor* Actor, FVector& OutLocalCenter, FVector& OutExtent)
	{
		OutLocalCenter = FVector(0.f, 0.f, 50.f);
		OutExtent = FVector(50.f);
		TArray<UStaticMeshComponent*> Meshes;
		GetBodyMeshes(Actor, Meshes);
		FBox Box(ForceInit);
		for (const UStaticMeshComponent* Mesh : Meshes)
		{
			Box += Mesh->Bounds.GetBox();
		}
		if (Actor && Box.IsValid)
		{
			OutLocalCenter = Actor->GetActorTransform().InverseTransformPosition(Box.GetCenter());
			OutExtent = Box.GetExtent();
		}
	}

	/**
	 * Adds a purely cosmetic engine basic shape (Cube/Cylinder/Sphere/Cone) to an actor.
	 * No collision, so it never blocks cursor traces or placement clicks.
	 */
	inline UStaticMeshComponent* AddVisualShape(AActor* Actor, const TCHAR* ShapeName, const FLinearColor& Color,
		const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& RelativeScale)
	{
		if (!Actor || !Actor->GetRootComponent())
		{
			return nullptr;
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), ShapeName, ShapeName));
		if (!Mesh)
		{
			return nullptr;
		}
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Actor, MakeUniqueObjectName(Actor, UStaticMeshComponent::StaticClass(), FName(ShapeName)));
		Comp->SetStaticMesh(Mesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCastShadow(false);
		Comp->SetupAttachment(Actor->GetRootComponent());
		Comp->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
		Comp->SetRelativeScale3D(RelativeScale);
		Actor->AddInstanceComponent(Comp);
		Comp->RegisterComponent();
		if (UMaterialInterface* Base = GetTintBaseMaterial())
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Comp);
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			Comp->SetMaterial(0, MID);
		}
		return Comp;
	}

	inline void SetShapeColor(UStaticMeshComponent* Comp, const FLinearColor& Color)
	{
		if (Comp)
		{
			if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0)))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	}
}
