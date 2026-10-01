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
	AActor* GetGameManager() const;
	UClass* GetSlotClass() const;

	
	bool CallGameManagerAmountFunction(FName FunctionName, int32 Amount, bool& bOutCalled) const;

	
	bool GetSlotRecord(const AActor* ClickedSlot, bool& bOutFound, bool& bOutOccupied) const;
	void WriteSlotRecord(const AActor* ClickedSlot, AActor* PlacedDefender);

	FTimerHandle SetupTimer;
	bool bInputBound = false;
	bool bCreatedDefenderBar = false;
	int32 BarSearchAttempts = 0;
};
