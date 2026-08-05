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

	// Server-authoritative entry points used by ANPCCharacter's command
	// functions (in turn triggered by UI buttons or a recognized voice
	// command). Safe to call at any time, from any NPC state.

	// Cancels any pending wander/waiting timer and stops movement. The NPC
	// stays in place; ANPCCharacter::Tick handles turning to face LookAtTarget.
	void CommandStandAndLook();

	// Cancels any pending timer and puts the NPC back into normal wandering.
	void CommandResumeWander();

	// Cancels any pending timer and starts continuously moving toward
	// Target, re-pathing periodically as it moves. Pass nullptr to just
	// stop (falls back to wandering).
	void CommandFollow(AActor* Target);

	UPROPERTY(EditAnywhere, Category = "Follow")
	float FollowAcceptanceRadius = 150.f;

	// How often to issue a fresh MoveToActor while following, in seconds.
	// Doesn't need to be every tick - the target only moves so fast.
	UPROPERTY(EditAnywhere, Category = "Follow")
	float FollowRepathInterval = 0.5f;

protected:
	void MoveToRandomLocation();

	void ResumeWandering();
	
	virtual void OnMoveCompleted(FAIRequestID RequestID,const FPathFollowingResult& Result) override;

private:

	FTimerHandle WanderTimer;

	float FollowRepathTimer = 0.f;

};
