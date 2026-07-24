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

	// Called (server-side) when a supported voice command lands within
	// listen range of this NPC. Interrupts wandering/waiting to clear
	// the path, then drifts back once done.
	void MoveAsideFrom(const FVector& ThreatLocation);

	FVector LastLocation;

	float StuckTimer = 0.f;

	UPROPERTY(EditAnywhere)
	float MinMovementDistance = 20.f;

	UPROPERTY(EditAnywhere)
	float MaxStuckTime = 3.f;

protected:
	void MoveToRandomLocation();

	void ResumeWandering();

	// Recovery when the pawn hasn't moved for MaxStuckTime, regardless of
	// which state it was stuck in.
	void HandleStuck();

	virtual void OnMoveCompleted(FAIRequestID RequestID,const FPathFollowingResult& Result) override;

private:

	FTimerHandle WanderTimer;

	// Where the NPC was standing right before it moved aside for a command,
	// so it can drift back afterwards instead of teleporting or staying put.
	FVector PreCommandLocation = FVector::ZeroVector;

};