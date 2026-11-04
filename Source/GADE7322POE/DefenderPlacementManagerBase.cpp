#include "DefenderPlacementManagerBase.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CollisionQueryParams.h"
#include "DefenderBarWidget.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "UnitReflectionUtils.h"
#include "UpgradeAuraComponent.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/Anchors.h"

DEFINE_LOG_CATEGORY_STATIC(LogDefenderPlacement, Log, All);

namespace DefenderPlacementPrivate
{
	static FProperty* FindStructField(const UStruct* Struct, const TCHAR* AuthoredName)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			if (Struct->GetAuthoredNameForField(*It).Equals(AuthoredName) || It->GetName().Equals(AuthoredName))
			{
				return *It;
			}
		}
		return nullptr;
	}

	static bool ReadOutputBool(UFunction* Fn, const uint8* Parms, bool DefaultValue)
	{
		for (TFieldIterator<FProperty> It(Fn); It && (It->PropertyFlags & CPF_Parm); ++It)
		{
			if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(*It))
			{
				if (It->PropertyFlags & (CPF_ReturnParm | CPF_OutParm))
				{
					return BoolProp->GetPropertyValue_InContainer(Parms);
				}
			}
		}
		return DefaultValue;
	}
}

ADefenderPlacementManagerBase::ADefenderPlacementManagerBase()
{
	DefenderBarWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/Bases/WBP_DefenderBarBase.WBP_DefenderBarBase_C")));
}

ADefenderPlacementManagerBase* ADefenderPlacementManagerBase::GetDefenderPlacementManager(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ADefenderPlacementManagerBase> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

void ADefenderPlacementManagerBase::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE))
	{
		PollSetup();
		World->GetTimerManager().SetTimer(SetupTimer, this, &ADefenderPlacementManagerBase::PollSetup, 0.25f, true);
	}
}


void ADefenderPlacementManagerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SetupTimer);
	}
	if (DefenderBarWidget && bCreatedDefenderBar)
	{
		DefenderBarWidget->RemoveFromParent();
	}
	DefenderBarWidget = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ADefenderPlacementManagerBase::PollSetup()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	if (!bInputBound)
	{
		// Pushed above the player controller's own input, so these bindings consume the clicks
		// that used to go straight to TryPlaceArcher from BP_TDPlayerController.
		EnableInput(PC);
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADefenderPlacementManagerBase::HandlePlaceClick);
			InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ADefenderPlacementManagerBase::HandleCancelInput);
			InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ADefenderPlacementManagerBase::HandleCancelInput);
			bInputBound = true;
		}
	}

	if (!DefenderBarWidget)
	{
		// Prefer a bar already placed in the HUD 
		TArray<UUserWidget*> ExistingBars;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ExistingBars, UDefenderBarWidget::StaticClass(), false);
		if (ExistingBars.Num() > 0)
		{
			DefenderBarWidget = ExistingBars[0];
		}
		else if (++BarSearchAttempts < 4)
		{
			return;
		}
	}

	if (!DefenderBarWidget)
	{
		if (UClass* BarClass = DefenderBarWidgetClass.LoadSynchronous())
		{
			DefenderBarWidget = CreateWidget<UUserWidget>(PC, BarClass);
			if (DefenderBarWidget)
			{
				bCreatedDefenderBar = true;
				DefenderBarWidget->AddToViewport(DefenderBarZOrder);
				DefenderBarWidget->SetAnchorsInViewport(FAnchors(0.5f, 1.f));
				DefenderBarWidget->SetAlignmentInViewport(FVector2D(0.5f, 1.f));
				DefenderBarWidget->SetPositionInViewport(FVector2D(0.f, -16.f), false);
			}
		}
	}

	if (bInputBound && DefenderBarWidget)
	{
		GetWorld()->GetTimerManager().ClearTimer(SetupTimer);
	}
}

AActor* ADefenderPlacementManagerBase::GetGameManager() const
{
	if (const FObjectProperty* Prop = FindFProperty<FObjectProperty>(GetClass(), FName(TEXT("GameManagerRef"))))
	{
		if (AActor* GM = Cast<AActor>(Prop->GetObjectPropertyValue_InContainer(this)); IsValid(GM))
		{
			return GM;
		}
	}

	static TWeakObjectPtr<UClass> GameManagerClass;
	if (!GameManagerClass.IsValid())
	{
		GameManagerClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Core/BP_GameManager.BP_GameManager_C"));
	}
	if (GameManagerClass.IsValid() && GetWorld())
	{
		for (TActorIterator<AActor> It(GetWorld(), GameManagerClass.Get()); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}
	}
	return nullptr;
}

UClass* ADefenderPlacementManagerBase::GetSlotClass() const
{
	if (const FClassProperty* Prop = FindFProperty<FClassProperty>(GetClass(), FName(TEXT("SlotClass"))))
	{
		if (UClass* SlotClass = Cast<UClass>(Prop->GetObjectPropertyValue_InContainer(this)))
		{
			return SlotClass;
		}
	}
	static TWeakObjectPtr<UClass> FallbackSlotClass;
	if (!FallbackSlotClass.IsValid())
	{
		FallbackSlotClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Defenders/BP_DefenderSlot.BP_DefenderSlot_C"));
	}
	return FallbackSlotClass.Get();
}

bool ADefenderPlacementManagerBase::IsGameplayActive() const
{
	AActor* GM = GetGameManager();
	if (!GM)
	{
		return true;
	}
	UFunction* IsPlayingFn = GM->FindFunction(FName(TEXT("IsPlaying")));
	if (!IsPlayingFn)
	{
		return true;
	}
	TArray<uint8> Parms;
	Parms.SetNumZeroed(FMath::Max<int32>(1, IsPlayingFn->ParmsSize));
	GM->ProcessEvent(IsPlayingFn, Parms.GetData());
	return DefenderPlacementPrivate::ReadOutputBool(IsPlayingFn, Parms.GetData(), true);
}

int32 ADefenderPlacementManagerBase::GetPlayerGold() const
{
	return FMath::RoundToInt(UnitReflection::GetNumeric(GetGameManager(), FName(TEXT("Gold")), 0.f));
}

bool ADefenderPlacementManagerBase::CallGameManagerAmountFunction(FName FunctionName, int32 Amount, bool& bOutCalled) const
{
	bOutCalled = false;
	AActor* GM = GetGameManager();
	UFunction* Fn = GM ? GM->FindFunction(FunctionName) : nullptr;
	if (!Fn)
	{
		return false;
	}

	TArray<uint8> Parms;
	Parms.SetNumZeroed(FMath::Max<int32>(1, Fn->ParmsSize));
	bool bAmountSet = false;
	for (TFieldIterator<FProperty> It(Fn); It && (It->PropertyFlags & CPF_Parm); ++It)
	{
		FProperty* Prop = *It;
		if (bAmountSet || (Prop->PropertyFlags & (CPF_ReturnParm | CPF_OutParm)))
		{
			continue;
		}
		if (FIntProperty* AsInt = CastField<FIntProperty>(Prop))
		{
			AsInt->SetPropertyValue_InContainer(Parms.GetData(), Amount);
			bAmountSet = true;
		}
		else if (FDoubleProperty* AsDouble = CastField<FDoubleProperty>(Prop))
		{
			AsDouble->SetPropertyValue_InContainer(Parms.GetData(), static_cast<double>(Amount));
			bAmountSet = true;
		}
		else if (FFloatProperty* AsFloat = CastField<FFloatProperty>(Prop))
		{
			AsFloat->SetPropertyValue_InContainer(Parms.GetData(), static_cast<float>(Amount));
			bAmountSet = true;
		}
	}
	if (!bAmountSet)
	{
		return false;
	}

	GM->ProcessEvent(Fn, Parms.GetData());
	bOutCalled = true;
	return DefenderPlacementPrivate::ReadOutputBool(Fn, Parms.GetData(), true);
}

bool ADefenderPlacementManagerBase::CanAffordDefender(int32 Cost) const
{
	if (Cost <= 0)
	{
		return true;
	}
	bool bCalled = false;
	const bool bHasEnough = CallGameManagerAmountFunction(FName(TEXT("HasEnoughGold")), Cost, bCalled);
	return bCalled ? bHasEnough : GetPlayerGold() >= Cost;
}

int32 ADefenderPlacementManagerBase::GetDefenderClassCost(TSubclassOf<AActor> DefenderClass, int32 FallbackCost)
{
	if (DefenderClass)
	{
		const int32 ClassCost = FMath::RoundToInt(UnitReflection::GetNumeric(DefenderClass->GetDefaultObject(), FName(TEXT("PlacementCost")), 0.f));
		if (ClassCost > 0)
		{
			return ClassCost;
		}
	}
	return FallbackCost;
}

bool ADefenderPlacementManagerBase::GetSlotRecord(const AActor* ClickedSlot, bool& bOutFound, bool& bOutOccupied) const
{
	bOutFound = false;
	bOutOccupied = false;

	const FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(GetClass(), FName(TEXT("Slots")));
	const FStructProperty* Inner = ArrayProp ? CastField<FStructProperty>(ArrayProp->Inner) : nullptr;
	if (!Inner)
	{
		return false;
	}
	const FObjectProperty* SlotActorProp = CastField<FObjectProperty>(DefenderPlacementPrivate::FindStructField(Inner->Struct, TEXT("SlotActor")));
	const FBoolProperty* OccupiedProp = CastField<FBoolProperty>(DefenderPlacementPrivate::FindStructField(Inner->Struct, TEXT("bOccupied")));
	if (!SlotActorProp)
	{
		return false;
	}

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));
	for (int32 Index = 0; Index < Helper.Num(); ++Index)
	{
		const uint8* Entry = Helper.GetRawPtr(Index);
		if (SlotActorProp->GetObjectPropertyValue_InContainer(Entry) == ClickedSlot)
		{
			bOutFound = true;
			bOutOccupied = OccupiedProp && OccupiedProp->GetPropertyValue_InContainer(Entry);
			return true;
		}
	}
	return true;
}

void ADefenderPlacementManagerBase::WriteSlotRecord(const AActor* ClickedSlot, AActor* PlacedDefender)
{
	FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(GetClass(), FName(TEXT("Slots")));
	FStructProperty* Inner = ArrayProp ? CastField<FStructProperty>(ArrayProp->Inner) : nullptr;
	if (!Inner)
	{
		return;
	}
	FObjectProperty* SlotActorProp = CastField<FObjectProperty>(DefenderPlacementPrivate::FindStructField(Inner->Struct, TEXT("SlotActor")));
	FBoolProperty* OccupiedProp = CastField<FBoolProperty>(DefenderPlacementPrivate::FindStructField(Inner->Struct, TEXT("bOccupied")));
	FObjectProperty* DefenderProp = CastField<FObjectProperty>(DefenderPlacementPrivate::FindStructField(Inner->Struct, TEXT("PlacedDefender")));
	if (!SlotActorProp)
	{
		return;
	}

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));
	for (int32 Index = 0; Index < Helper.Num(); ++Index)
	{
		uint8* Entry = Helper.GetRawPtr(Index);
		if (SlotActorProp->GetObjectPropertyValue_InContainer(Entry) != ClickedSlot)
		{
			continue;
		}
		if (OccupiedProp)
		{
			OccupiedProp->SetPropertyValue_InContainer(Entry, true);
		}
		if (DefenderProp && PlacedDefender && PlacedDefender->IsA(DefenderProp->PropertyClass))
		{
			DefenderProp->SetObjectPropertyValue_InContainer(Entry, PlacedDefender);
		}
		return;
	}
}

bool ADefenderPlacementManagerBase::IsSlotAvailable(AActor* ClickedSlot) const
{
	if (!IsValid(ClickedSlot))
	{
		return false;
	}
	if (UClass* SlotClass = GetSlotClass(); SlotClass && !ClickedSlot->IsA(SlotClass))
	{
		return false;
	}
	if (const FBoolProperty* OccupiedProp = FindFProperty<FBoolProperty>(ClickedSlot->GetClass(), FName(TEXT("bOccupied"))))
	{
		if (OccupiedProp->GetPropertyValue_InContainer(ClickedSlot))
		{
			return false;
		}
	}

	bool bFound = false;
	bool bRecordOccupied = false;
	if (GetSlotRecord(ClickedSlot, bFound, bRecordOccupied))
	{
		// Missing from the Slots array still counts as free if the pad actor itself is unoccupied.
		return !bRecordOccupied;
	}
	return true;
}

AActor* ADefenderPlacementManagerBase::ResolveSlotFromActor(AActor* HitActor) const
{
	UClass* SlotClass = GetSlotClass();
	if (!IsValid(HitActor) || !SlotClass)
	{
		return nullptr;
	}

	for (AActor* Cursor = HitActor; IsValid(Cursor); Cursor = Cursor->GetAttachParentActor())
	{
		if (Cursor->IsA(SlotClass))
		{
			return Cursor;
		}
	}

	// Terrain tiles often store their pad on a DefenderSlot object property.
	if (const FObjectProperty* SlotProp = FindFProperty<FObjectProperty>(HitActor->GetClass(), FName(TEXT("DefenderSlot"))))
	{
		if (AActor* LinkedSlot = Cast<AActor>(SlotProp->GetObjectPropertyValue_InContainer(HitActor)))
		{
			if (LinkedSlot->IsA(SlotClass))
			{
				return LinkedSlot;
			}
		}
	}

	return nullptr;
}

AActor* ADefenderPlacementManagerBase::FindNearestSlot(const FVector& WorldLocation) const
{
	UWorld* World = GetWorld();
	UClass* SlotClass = GetSlotClass();
	if (!World || !SlotClass)
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(350.f);
	for (TActorIterator<AActor> It(World, SlotClass); It; ++It)
	{
		AActor* Slot = *It;
		if (!IsValid(Slot) || !IsSlotAvailable(Slot))
		{
			continue;
		}
		const float DistSq = static_cast<float>(FVector::DistSquared2D(WorldLocation, Slot->GetActorLocation()));
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Slot;
		}
	}
	return Best;
}

AActor* ADefenderPlacementManagerBase::FindSlotUnderCursor() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return nullptr;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return nullptr;
	}

	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DefenderSlotCursor), true);
	Params.bReturnPhysicalMaterial = false;

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, WorldOrigin, TraceEnd, ECC_Visibility, Params);
	if (Hits.Num() == 0)
	{
		World->LineTraceMultiByChannel(Hits, WorldOrigin, TraceEnd, ECC_WorldStatic, Params);
	}

	for (const FHitResult& Hit : Hits)
	{
		if (AActor* Slot = ResolveSlotFromActor(Hit.GetActor()))
		{
			return Slot;
		}
	}

	// Fallback: click landed on the ground near a pad (tile mesh blocked the pad itself).
	FHitResult GroundHit;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, true, GroundHit) ||
		PC->GetHitResultUnderCursor(ECC_WorldStatic, true, GroundHit))
	{
		if (AActor* Slot = ResolveSlotFromActor(GroundHit.GetActor()))
		{
			return Slot;
		}
		return FindNearestSlot(GroundHit.ImpactPoint);
	}

	return nullptr;
}

void ADefenderPlacementManagerBase::SetPlacementCursor(bool bPlacing) const
{
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PC->CurrentMouseCursor = bPlacing ? EMouseCursor::Crosshairs : EMouseCursor::Default;
	}
}

void ADefenderPlacementManagerBase::PostMessage(const FString& Message, bool bIsError)
{
	UE_LOG(LogDefenderPlacement, Log, TEXT("%s"), *Message);
	OnDefenderPlacementMessage.Broadcast(FText::FromString(Message), bIsError);
}

bool ADefenderPlacementManagerBase::SelectDefender(TSubclassOf<AActor> DefenderClass, int32 Cost, FText DisplayName)
{
	if (!IsGameplayActive())
	{
		return false;
	}
	if (!DefenderClass)
	{
		PostMessage(FString::Printf(TEXT("%s is not available yet"), *DisplayName.ToString()), true);
		return false;
	}

	Cost = FMath::Max(0, Cost);
	if (!CanAffordDefender(Cost))
	{
		PostMessage(FString::Printf(TEXT("Not enough gold for %s (need %d, have %d)"), *DisplayName.ToString(), Cost, GetPlayerGold()), true);
		return false;
	}

	SelectedDefenderClass = DefenderClass;
	SelectedDefenderCost = Cost;
	SelectedUpgradeCost = 0;
	SelectedDefenderName = DisplayName;
	bIsPlacingDefender = true;
	bIsUpgradingDefender = false;
	SetPlacementCursor(true);
	OnDefenderSelectionChanged.Broadcast();
	return true;
}

void ADefenderPlacementManagerBase::ClearSelection()
{
	SelectedDefenderClass = nullptr;
	SelectedDefenderCost = 0;
	SelectedUpgradeCost = 0;
	SelectedDefenderName = FText::GetEmpty();
	bIsPlacingDefender = false;
	bIsUpgradingDefender = false;
	SetPlacementCursor(false);
	OnDefenderSelectionChanged.Broadcast();
}

void ADefenderPlacementManagerBase::CancelDefenderPlacement()
{
	if (!bIsPlacingDefender && !bIsUpgradingDefender)
	{
		return;
	}
	const bool bWasUpgrading = bIsUpgradingDefender;
	ClearSelection();
	PostMessage(bWasUpgrading ? TEXT("Upgrade cancelled") : TEXT("Placement cancelled"), false);
}

void ADefenderPlacementManagerBase::HandleCancelInput()
{
	CancelDefenderPlacement();
}

bool ADefenderPlacementManagerBase::IsDefenderUpgraded(const AActor* Defender)
{
	if (!IsValid(Defender))
	{
		return false;
	}
	if (const UUpgradeAuraComponent* Aura = Defender->FindComponentByClass<UUpgradeAuraComponent>())
	{
		return Aura->IsAuraVisible();
	}
	return false;
}

bool ADefenderPlacementManagerBase::SpendPlayerGold(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	if (!CanAffordDefender(Amount))
	{
		return false;
	}

	const int32 GoldBefore = GetPlayerGold();
	bool bCalled = false;
	bool bSpent = CallGameManagerAmountFunction(FName(TEXT("SpendGold")), Amount, bCalled);
	if (bCalled && !bSpent)
	{
		bSpent = GetPlayerGold() <= GoldBefore - Amount;
	}
	if (!bCalled)
	{
		const int32 Gold = GetPlayerGold();
		bSpent = Gold >= Amount;
		if (bSpent)
		{
			UnitReflection::SetNumeric(GetGameManager(), FName(TEXT("Gold")), static_cast<float>(Gold - Amount));
		}
	}
	return bSpent;
}

void ADefenderPlacementManagerBase::ApplyUpgradeStats(AActor* Defender) const
{
	if (!IsValid(Defender))
	{
		return;
	}

	if (UActorComponent* Health = UnitReflection::FindHealthComponent(Defender))
	{
		const float MaxHealth = UnitReflection::GetNumeric(Health, FName(TEXT("MaxHealth")), 100.f);
		const float Current = UnitReflection::GetNumeric(Health, FName(TEXT("CurrentHealth")), MaxHealth);
		const float NewMax = FMath::Max(1.f, MaxHealth * UpgradeHealthMultiplier);
		const float Ratio = MaxHealth > 0.f ? Current / MaxHealth : 1.f;
		UnitReflection::SetNumeric(Health, FName(TEXT("MaxHealth")), NewMax);
		UnitReflection::SetNumeric(Health, FName(TEXT("CurrentHealth")), NewMax * Ratio);
		UnitReflection::SetNumeric(Defender, FName(TEXT("MaxHealth")), NewMax);
		UnitReflection::SetNumeric(Defender, FName(TEXT("CurrentHealth")), NewMax * Ratio);
	}

	if (UnitReflection::HasNumeric(Defender, FName(TEXT("AttackDamage"))))
	{
		const float Damage = UnitReflection::GetNumeric(Defender, FName(TEXT("AttackDamage")), 0.f);
		UnitReflection::SetNumeric(Defender, FName(TEXT("AttackDamage")), Damage * UpgradeDamageMultiplier);
	}
}

bool ADefenderPlacementManagerBase::BeginUpgradeMode(TSubclassOf<AActor> DefenderClass, int32 UpgradeCost, FText DisplayName)
{
	if (!IsGameplayActive())
	{
		return false;
	}
	if (!DefenderClass)
	{
		PostMessage(FString::Printf(TEXT("%s is not available yet"), *DisplayName.ToString()), true);
		return false;
	}

	UpgradeCost = FMath::Max(0, UpgradeCost);
	if (!CanAffordDefender(UpgradeCost))
	{
		PostMessage(FString::Printf(TEXT("Not enough gold to upgrade %s (need %d, have %d)"),
			*DisplayName.ToString(), UpgradeCost, GetPlayerGold()), true);
		return false;
	}

	SelectedDefenderClass = DefenderClass;
	SelectedDefenderCost = 0;
	SelectedUpgradeCost = UpgradeCost;
	SelectedDefenderName = DisplayName;
	bIsPlacingDefender = false;
	bIsUpgradingDefender = true;
	SetPlacementCursor(true);
	OnDefenderSelectionChanged.Broadcast();
	PostMessage(FString::Printf(TEXT("Upgrade mode: click a placed %s (^^ %d ^^)"),
		*DisplayName.ToString(), UpgradeCost), false);
	return true;
}

AActor* ADefenderPlacementManagerBase::FindDefenderUnderCursor(TSubclassOf<AActor> DefenderClass) const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	UWorld* World = GetWorld();
	if (!PC || !World || !DefenderClass)
	{
		return nullptr;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return nullptr;
	}

	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DefenderUpgradeCursor), true);

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, WorldOrigin, TraceEnd, ECC_Visibility, Params);
	if (Hits.Num() == 0)
	{
		World->LineTraceMultiByChannel(Hits, WorldOrigin, TraceEnd, ECC_WorldStatic, Params);
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (IsValid(HitActor) && HitActor->IsA(DefenderClass))
		{
			return HitActor;
		}
	}

	FHitResult GroundHit;
	FVector SearchPoint = FVector::ZeroVector;
	bool bHavePoint = false;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, true, GroundHit) ||
		PC->GetHitResultUnderCursor(ECC_WorldStatic, true, GroundHit))
	{
		if (IsValid(GroundHit.GetActor()) && GroundHit.GetActor()->IsA(DefenderClass))
		{
			return GroundHit.GetActor();
		}
		SearchPoint = GroundHit.ImpactPoint;
		bHavePoint = true;
	}
	if (!bHavePoint)
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(400.f);
	for (TActorIterator<AActor> It(World, DefenderClass); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}
		const float DistSq = static_cast<float>(FVector::DistSquared2D(SearchPoint, Candidate->GetActorLocation()));
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}
	return Best;
}

bool ADefenderPlacementManagerBase::TryUpgradeDefender(AActor* Defender)
{
	if (!IsValid(Defender) || !SelectedDefenderClass || !Defender->IsA(SelectedDefenderClass))
	{
		PostMessage(TEXT("Click a matching placed defender to upgrade"), true);
		return false;
	}
	if (IsDefenderUpgraded(Defender))
	{
		PostMessage(TEXT("That defender is already upgraded"), true);
		return false;
	}
	if (!CanAffordDefender(SelectedUpgradeCost))
	{
		PostMessage(FString::Printf(TEXT("Not enough gold (need %d, have %d)"), SelectedUpgradeCost, GetPlayerGold()), true);
		return false;
	}
	if (!SpendPlayerGold(SelectedUpgradeCost))
	{
		PostMessage(TEXT("Could not spend gold for upgrade"), true);
		return false;
	}

	ApplyUpgradeStats(Defender);
	UUpgradeAuraComponent::ApplyTo(Defender);

	const FString Name = SelectedDefenderName.IsEmpty() ? Defender->GetName() : SelectedDefenderName.ToString();
	const int32 Spent = SelectedUpgradeCost;
	UE_LOG(LogDefenderPlacement, Warning, TEXT("Upgraded %s for %d gold (aura applied)"), *GetNameSafe(Defender), Spent);

	ClearSelection();
	PostMessage(FString::Printf(TEXT("%s upgraded! (-%d gold)"), *Name, Spent), false);
	return true;
}

void ADefenderPlacementManagerBase::HandlePlaceClick()
{
	if (!IsGameplayActive())
	{
		return;
	}

	if (bIsUpgradingDefender)
	{
		AActor* Defender = FindDefenderUnderCursor(SelectedDefenderClass);
		if (!Defender)
		{
			PostMessage(TEXT("Click a placed defender to upgrade"), true);
			return;
		}
		TryUpgradeDefender(Defender);
		return;
	}

	if (!bIsPlacingDefender)
	{
		return;
	}

	AActor* ClickedSlot = FindSlotUnderCursor();
	if (!ClickedSlot)
	{
		PostMessage(TEXT("Invalid location - click a free defender pad"), true);
		return;
	}
	TryPlaceDefender(ClickedSlot, SelectedDefenderClass, SelectedDefenderCost);
}

bool ADefenderPlacementManagerBase::TryPlaceDefender(AActor* ClickedSlot, TSubclassOf<AActor> DefenderClass, int32 Cost)
{
	UWorld* World = GetWorld();
	if (!World || !DefenderClass)
	{
		PostMessage(TEXT("No defender selected"), true);
		return false;
	}
	Cost = FMath::Max(0, Cost);
	const FString DefenderName = (bIsPlacingDefender && DefenderClass == SelectedDefenderClass && !SelectedDefenderName.IsEmpty())
		? SelectedDefenderName.ToString()
		: DefenderClass->GetName();

	//Location: must be a registered, unoccupied pad (pads only exist on defender tiles, never on the path).
	UClass* SlotClass = GetSlotClass();
	if (!IsValid(ClickedSlot) || (SlotClass && !ClickedSlot->IsA(SlotClass)))
	{
		PostMessage(TEXT("Invalid location - click a free defender pad"), true);
		return false;
	}
	if (!IsSlotAvailable(ClickedSlot))
	{
		PostMessage(TEXT("That pad is already occupied"), true);
		return false;
	}

	//Gold.
	if (!CanAffordDefender(Cost))
	{
		PostMessage(FString::Printf(TEXT("Not enough gold for %s (need %d, have %d)"), *DefenderName, Cost, GetPlayerGold()), true);
		return false;
	}

	//Spawn.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Defender = World->SpawnActor<AActor>(DefenderClass, ClickedSlot->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	if (!IsValid(Defender))
	{
		PostMessage(FString::Printf(TEXT("Could not place %s"), *DefenderName), true);
		return false;
	}

	//Spend gold only after the spawn succeeded; roll the spawn back if the spend is refused.
	if (Cost > 0)
	{
		const int32 GoldBefore = GetPlayerGold();
		bool bCalled = false;
		bool bSpent = CallGameManagerAmountFunction(FName(TEXT("SpendGold")), Cost, bCalled);
		if (bCalled && !bSpent)
		{
			// BP SpendGold evaluates its success output after subtracting, so it reports false
			// whenever the remaining gold is below the cost even though the gold was taken.
			bSpent = GetPlayerGold() <= GoldBefore - Cost;
		}
		if (!bCalled)
		{
			const int32 Gold = GetPlayerGold();
			bSpent = Gold >= Cost;
			if (bSpent)
			{
				UnitReflection::SetNumeric(GetGameManager(), FName(TEXT("Gold")), static_cast<float>(Gold - Cost));
			}
		}
		if (!bSpent)
		{
			Defender->Destroy();
			PostMessage(FString::Printf(TEXT("Not enough gold for %s"), *DefenderName), true);
			return false;
		}
	}

	//Occupy the slot through the existing slot actor + manager Slots record.
	if (UFunction* SetOccupiedFn = ClickedSlot->FindFunction(FName(TEXT("SetOccupied"))))
	{
		TArray<uint8> Parms;
		Parms.SetNumZeroed(FMath::Max<int32>(1, SetOccupiedFn->ParmsSize));
		for (TFieldIterator<FProperty> It(SetOccupiedFn); It && (It->PropertyFlags & CPF_Parm); ++It)
		{
			if (FBoolProperty* BoolProp = CastField<FBoolProperty>(*It))
			{
				BoolProp->SetPropertyValue_InContainer(Parms.GetData(), true);
				break;
			}
		}
		ClickedSlot->ProcessEvent(SetOccupiedFn, Parms.GetData());
	}
	else if (FBoolProperty* OccupiedProp = FindFProperty<FBoolProperty>(ClickedSlot->GetClass(), FName(TEXT("bOccupied"))))
	{
		OccupiedProp->SetPropertyValue_InContainer(ClickedSlot, true);
	}
	WriteSlotRecord(ClickedSlot, Defender);

	UE_LOG(LogDefenderPlacement, Warning, TEXT("Placed %s on %s for %d gold (gold now %d)"),
		*GetNameSafe(Defender), *GetNameSafe(ClickedSlot), Cost, GetPlayerGold());

	//Leave placement mode.
	if (bIsPlacingDefender)
	{
		ClearSelection();
	}
	PostMessage(FString::Printf(TEXT("%s placed (-%d gold)"), *DefenderName, Cost), false);
	return true;
}
