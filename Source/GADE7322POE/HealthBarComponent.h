#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "HealthBarComponent.generated.h"

class UHealthBarWidget;
class UActorComponent;


UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class GADE7322POE_API UHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UHealthBarComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealthSource(UActorComponent* InHealthComponent);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void RefreshFromHealthSource();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	FVector WorldOffset = FVector(0.f, 0.f, 140.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bHideOwnerHealthText = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bShowNumericText = false;

protected:
	UActorComponent* FindOwnerHealthComponent() const;
	void HideOwnerHealthText() const;
	void ConfigureWidgetComponent();

	UPROPERTY()
	TObjectPtr<UActorComponent> HealthSource;

	UPROPERTY()
	TObjectPtr<UHealthBarWidget> HealthBarWidget;
};
