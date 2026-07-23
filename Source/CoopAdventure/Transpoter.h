// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Transpoter.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COOPADVENTURE_API UTranspoter : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UTranspoter();

protected:
	
	virtual void BeginPlay() override;

public:	
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
	FVector StartPoint;
	

	FVector EndPoint;
	
	
	bool ArePointsSet;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MoveTime;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<AActor*> TriggerActors;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	int ActivatedTriggerCount;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	bool AllTriggerActorsTriggered;

	UFUNCTION(BlueprintCallable)
	void Setpoints(FVector Point1,FVector Point2);

	UFUNCTION()
	void OnPressurePlateActivated();
	UFUNCTION()
	void OnPressurePlateDeactivated();
	
		
};
