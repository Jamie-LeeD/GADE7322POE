#include "HealthBarComponent.h"

#include "HealthBarWidget.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace HealthBarPrivate
{
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
		return DefaultValue;
	}

	static bool HasCurrentHealth(UActorComponent* Comp)
	{
		return Comp && (
			FindFProperty<FDoubleProperty>(Comp->GetClass(), FName(TEXT("CurrentHealth"))) != nullptr ||
			FindFProperty<FFloatProperty>(Comp->GetClass(), FName(TEXT("CurrentHealth"))) != nullptr);
	}
}

UHealthBarComponent::UHealthBarComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);

	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetPivot(FVector2D(0.5f, 1.f));
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
}

void UHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();
	ConfigureWidgetComponent();

	if (!HealthSource)
	{
		HealthSource = FindOwnerHealthComponent();
	}

	if (bHideOwnerHealthText)
	{
		HideOwnerHealthText();
	}

	RefreshFromHealthSource();
}

void UHealthBarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AActor* Owner = GetOwner())
	{
		SetWorldLocation(Owner->GetActorLocation() + WorldOffset);
	}

	RefreshFromHealthSource();
}

void UHealthBarComponent::SetHealthSource(UActorComponent* InHealthComponent)
{
	HealthSource = InHealthComponent;
	RefreshFromHealthSource();
}

void UHealthBarComponent::ConfigureWidgetComponent()
{
	SetWidgetClass(UHealthBarWidget::StaticClass());
	InitWidget();
	HealthBarWidget = Cast<UHealthBarWidget>(GetWidget());
	if (HealthBarWidget)
	{
		HealthBarWidget->SetShowNumericText(bShowNumericText);
	}
}

UActorComponent* UHealthBarComponent::FindOwnerHealthComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(Components);

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
			if (HealthBarPrivate::HasCurrentHealth(Comp))
			{
				return Comp;
			}
		}
	}

	for (UActorComponent* Comp : Components)
	{
		if (HealthBarPrivate::HasCurrentHealth(Comp))
		{
			return Comp;
		}
	}

	for (TFieldIterator<FObjectProperty> It(Owner->GetClass()); It; ++It)
	{
		FObjectProperty* Prop = *It;
		if (!Prop->GetName().Equals(TEXT("Health")))
		{
			continue;
		}
		if (UActorComponent* Comp = Cast<UActorComponent>(Prop->GetObjectPropertyValue_InContainer(Owner)))
		{
			if (HealthBarPrivate::HasCurrentHealth(Comp))
			{
				return Comp;
			}
		}
	}

	return nullptr;
}

void UHealthBarComponent::HideOwnerHealthText() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UTextRenderComponent*> TextComps;
	Owner->GetComponents<UTextRenderComponent>(TextComps);
	for (UTextRenderComponent* TextComp : TextComps)
	{
		if (TextComp && TextComp->GetName().Contains(TEXT("HealthText")))
		{
			TextComp->SetVisibility(false);
			TextComp->SetHiddenInGame(true);
		}
	}
}

void UHealthBarComponent::RefreshFromHealthSource()
{
	if (!HealthSource)
	{
		HealthSource = FindOwnerHealthComponent();
	}
	if (!HealthBarWidget)
	{
		HealthBarWidget = Cast<UHealthBarWidget>(GetWidget());
	}
	if (!HealthSource || !HealthBarWidget)
	{
		return;
	}

	const float MaxHealth = HealthBarPrivate::GetNumeric(HealthSource, FName(TEXT("MaxHealth")), 100.f);
	const float Current = HealthBarPrivate::GetNumeric(HealthSource, FName(TEXT("CurrentHealth")), MaxHealth);
	HealthBarWidget->SetHealth(Current, MaxHealth);

	const bool bDead = Current <= 0.f;
	SetVisibility(!bDead);
}
