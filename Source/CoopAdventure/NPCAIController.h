// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NPCAIController.generated.h"

UCLASS()
class COOPADVENTURE_API ANPCAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANPCAIController();

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FVector LastLocation;

	float StuckTimer = 0.f;

	UPROPERTY(EditAnywhere)
	float MinMovementDistance = 20.f;

	UPROPERTY(EditAnywhere)
	float MaxStuckTime = 3.f;

protected:
	void MoveToRandomLocation();

	void ResumeWandering();

	
	virtual void OnMoveCompleted(FAIRequestID RequestID,const FPathFollowingResult& Result) override;

private:

	FTimerHandle WanderTimer;

};
