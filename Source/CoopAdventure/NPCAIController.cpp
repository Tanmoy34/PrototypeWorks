// Fill out your copyright notice in the Description page of Project Settings.


#include "NPCAIController.h"
#include "NPCCharacter.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ANPCAIController::ANPCAIController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
}

// Called when the game starts or when spawned
void ANPCAIController::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())return;

	MoveToRandomLocation();

	
}

// Called every frame
void ANPCAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()) return;

	APawn* MyPawn = GetPawn();

	if (!MyPawn) return;

	// Don't run the stuck-detector while the NPC has been deliberately
	// told to stand still - it isn't "stuck", it's obeying a command.
	ANPCCharacter* NPC = Cast<ANPCCharacter>(MyPawn);
	if (NPC && NPC->GetState() == ENPCState::Idle)
	{
		StuckTimer = 0.f;
		LastLocation = MyPawn->GetActorLocation();
		return;
	}

	float Distance =FVector::Distance(
			MyPawn->GetActorLocation(),
			LastLocation);

	if (Distance < MinMovementDistance)
	{
		StuckTimer += DeltaTime;

		if (StuckTimer >= MaxStuckTime)
		{
			StopMovement();

			MoveToRandomLocation();

			StuckTimer = 0.f;
		}
	}
	else
	{
		StuckTimer = 0.f;
	}

	LastLocation = MyPawn->GetActorLocation();
}

void ANPCAIController::MoveToRandomLocation()
{
	if (!HasAuthority()) return;

	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());

	if (!NPC) return;

	// Don't wander unless we're actually in the Wandering state.
	if (NPC->GetState() != ENPCState::wandering) return;

	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSys) return;

	FNavLocation RandomPoint;

	if (NavSys->GetRandomReachablePointInRadius(NPC->GetActorLocation(),NPC->WanderRadius,RandomPoint))
	{
		MoveToLocation(RandomPoint.Location);
	}
}

void ANPCAIController::ResumeWandering()
{
	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());

	if (!NPC)
		return;

	if (NPC->GetState() != ENPCState::Waiting)
		return;

	NPC->SetState(ENPCState::wandering);

	MoveToRandomLocation();
}

void ANPCAIController::CommandStandAndLook()
{
	if (!HasAuthority()) return;

	GetWorld()->GetTimerManager().ClearTimer(WanderTimer);

	StopMovement();

	StuckTimer = 0.f;
}

void ANPCAIController::CommandResumeWander()
{
	if (!HasAuthority()) return;

	GetWorld()->GetTimerManager().ClearTimer(WanderTimer);

	StuckTimer = 0.f;

	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());
	if (!NPC) return;

	// SetState first so MoveToRandomLocation's internal wandering-state
	// check passes.
	NPC->SetState(ENPCState::wandering);

	MoveToRandomLocation();
}

void ANPCAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	if (!HasAuthority()) return;

	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());

	if (!NPC) return;

	switch (NPC->GetState())
	{
	case ENPCState::wandering:
		{
			NPC->SetState(ENPCState::Waiting);

			float WaitTime = FMath::FRandRange(
				NPC->MinWaitTime,
				NPC->MaxWaitTime);

			GetWorld()->GetTimerManager().SetTimer(
				WanderTimer,
				this,
				&ANPCAIController::ResumeWandering,
				WaitTime,
				false);

			break;
		}

	case ENPCState::MoveAside:
		{
			// We'll implement this later.
			break;
		}

	case ENPCState::Returning:
		{
			// We'll implement this later.
			break;
		}

	default:
		break;
	}
}
