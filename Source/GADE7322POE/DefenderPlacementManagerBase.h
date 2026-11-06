#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DefenderPlacementManagerBase.generated.h"

class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDefenderSelectionChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDefenderPlacementMessage, const FText&, Message, bool, bIsError);


UCLASS(Blueprintable)
class GADE7322POE_API ADefenderPlacementManagerBase : public AActor
{
	GENERATED_BODY()

public:
	ADefenderPlacementManagerBase();

	UFUNCTION(BlueprintPure, Category = "Defender Placement", meta = (WorldContext = "WorldContextObject"))
	static ADefenderPlacementManagerBase* GetDefenderPlacementManager(const UObject* WorldContextObject);

	
	UFUNCTION(BlueprintCallable, Category = "Defender Placement")
	bool SelectDefender(TSubclassOf<AActor> DefenderClass, int32 Cost, FText DisplayName);

	
	UFUNCTION(BlueprintCallable, Category = "Defender Placement")
	void CancelDefenderPlacement();

	
	UFUNCTION(BlueprintCallable, Category = "Defender Placement")
	bool TryPlaceDefender(AActor* ClickedSlot, TSubclassOf<AActor> DefenderClass, int32 Cost);

	/** Instant type-wide upgrade: spends gold once and upgrades every placed defender of this class. */
	UFUNCTION(BlueprintCallable, Category = "Defender Placement|Upgrade")
	bool PurchaseClassUpgrade(TSubclassOf<AActor> DefenderClass, int32 BaseUpgradeCost, FText DisplayName);

	/** Legacy: enters click-to-upgrade mode (kept for Blueprint compatibility). Prefer PurchaseClassUpgrade. */
	UFUNCTION(BlueprintCallable, Category = "Defender Placement|Upgrade")
	bool BeginUpgradeMode(TSubclassOf<AActor> DefenderClass, int32 UpgradeCost, FText DisplayName);

	UFUNCTION(BlueprintCallable, Category = "Defender Placement|Upgrade")
	bool TryUpgradeDefender(AActor* Defender);

	UFUNCTION(BlueprintPure, Category = "Defender Placement|Upgrade")
	static bool IsDefenderUpgraded(const AActor* Defender);

	UFUNCTION(BlueprintPure, Category = "Defender Placement|Upgrade")
	int32 GetClassUpgradeLevel(TSubclassOf<AActor> DefenderClass) const;

	UFUNCTION(BlueprintPure, Category = "Defender Placement|Upgrade")
	int32 GetNextUpgradeCost(TSubclassOf<AActor> DefenderClass, int32 BaseUpgradeCost) const;

	UFUNCTION(BlueprintPure, Category = "Defender Placement|Upgrade")
	bool IsClassUpgradeMaxed(TSubclassOf<AActor> DefenderClass) const;

	UFUNCTION(BlueprintPure, Category = "Defender Placement")
	bool IsSlotAvailable(AActor* ClickedSlot) const;

	UFUNCTION(BlueprintPure, Category = "Defender Placement|Gold")
	int32 GetPlayerGold() const;

	UFUNCTION(BlueprintPure, Category = "Defender Placement|Gold")
	bool CanAffordDefender(int32 Cost) const;

	UFUNCTION(BlueprintPure, Category = "Defender Placement")
	bool IsGameplayActive() const;

	
	UFUNCTION(BlueprintPure, Category = "Defender Placement")
	static int32 GetDefenderClassCost(TSubclassOf<AActor> DefenderClass, int32 FallbackCost);

	UPROPERTY(BlueprintReadOnly, Category = "Defender Placement|Selection")
	TSubclassOf<AActor> SelectedDefenderClass;

	UPROPERTY(BlueprintReadOnly, Category = "Defender Placement|Selection")
	int32 SelectedDefenderCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Defender Placement|Selection")
	FText SelectedDefenderName;

	UPROPERTY(BlueprintReadOnly, Category = "Defender Placement|Selection")
	bool bIsPlacingDefender = false;

	UPROPERTY(BlueprintReadOnly, Category = "Defender Placement|Upgrade")
	bool bIsUpgradingDefender = false;

	UPROPERTY(BlueprintReadOnly, Category = "Defender Placement|Upgrade")
	int32 SelectedUpgradeCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Placement|Upgrade", meta = (ClampMin = "1.0"))
	float UpgradeHealthMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Placement|Upgrade", meta = (ClampMin = "1.0"))
	float UpgradeDamageMultiplier = 1.2f;

	/** How many type-wide upgrade purchases are allowed per defender class (then button greys out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Placement|Upgrade", meta = (ClampMin = "1"))
	int32 MaxClassUpgradeLevel = 2;

	/** Next upgrade cost = BaseCost * (CostGrowth ^ currentLevel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defender Placement|Upgrade", meta = (ClampMin = "1.0"))
	float UpgradeCostGrowth = 1.75f;

	UPROPERTY(BlueprintAssignable, Category = "Defender Placement")
	FOnDefenderSelectionChanged OnDefenderSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Defender Placement")
	FOnDefenderPlacementMessage OnDefenderPlacementMessage;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defender Placement|UI")
	TSoftClassPtr<UUserWidget> DefenderBarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defender Placement|UI")
	int32 DefenderBarZOrder = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Defender Placement|UI")
	TObjectPtr<UUserWidget> DefenderBarWidget;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void PollSetup();
	void HandlePlaceClick();
	void HandleCancelInput();

	void ClearSelection();
	void PostMessage(const FString& Message, bool bIsError);
	void SetPlacementCursor(bool bPlacing) const;

	AActor* FindSlotUnderCursor() const;
	AActor* FindDefenderUnderCursor(TSubclassOf<AActor> DefenderClass) const;
	AActor* ResolveSlotFromActor(AActor* HitActor) const;
	AActor* FindNearestSlot(const FVector& WorldLocation) const;
	AActor* GetGameManager() const;
	UClass* GetSlotClass() const;

	bool SpendPlayerGold(int32 Amount);
	void ApplyUpgradeStats(AActor* Defender) const;
	void ApplyOwnedClassUpgrades(AActor* Defender) const;
	int32 UpgradeAllPlacedOfClass(TSubclassOf<AActor> DefenderClass) const;

	/** Purchased type-wide upgrade count per defender class. */
	TMap<TSubclassOf<AActor>, int32> ClassUpgradeLevels;

	
	bool CallGameManagerAmountFunction(FName FunctionName, int32 Amount, bool& bOutCalled) const;

	
	bool GetSlotRecord(const AActor* ClickedSlot, bool& bOutFound, bool& bOutOccupied) const;
	void WriteSlotRecord(const AActor* ClickedSlot, AActor* PlacedDefender);

	/** Hide the pad cube / marker on the slot (and linked tile) once a defender is placed. */
	void SetSlotPlacementVisualOccupied(AActor* Slot, bool bOccupied) const;

	FTimerHandle SetupTimer;
	bool bInputBound = false;
	bool bCreatedDefenderBar = false;
	int32 BarSearchAttempts = 0;
};
