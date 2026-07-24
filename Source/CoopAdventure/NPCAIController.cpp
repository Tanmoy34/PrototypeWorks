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

	if (!HasAuthority())
		return;

	APawn* MyPawn = GetPawn();

	if (!MyPawn)
		return;

	float Distance =
		FVector::Distance(
			MyPawn->GetActorLocation(),
			LastLocation);

	if (Distance < MinMovementDistance)
	{
		StuckTimer += DeltaTime;

		if (StuckTimer >= MaxStuckTime)
		{
			StopMovement();

			HandleStuck();

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
	if (!HasAuthority())
		return;

	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());

	if (!NPC)
		return;

	// Don't wander unless we're actually in the Wandering state.
	if (NPC->GetState() != ENPCState::Wandering)
		return;

	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSys)
		return;

	FNavLocation RandomPoint;

	if (NavSys->GetRandomReachablePointInRadius(
			NPC->GetActorLocation(),
			NPC->WanderRadius,
			RandomPoint))
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

	NPC->SetState(ENPCState::Wandering);

	MoveToRandomLocation();
}

void ANPCAIController::MoveAsideFrom(const FVector& ThreatLocation)
{
	if (!HasAuthority())
		return;

	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());

	if (!NPC)
		return;

	// Already clearing a path or drifting back — don't interrupt with a
	// second command.
	if (NPC->GetState() == ENPCState::MoveAside || NPC->GetState() == ENPCState::Returning)
		return;

	// A pending "resume wandering after waiting" timer no longer applies.
	GetWorld()->GetTimerManager().ClearTimer(WanderTimer);

	// Remember where we were so we can drift back once the path is clear.
	PreCommandLocation = NPC->GetActorLocation();

	FVector AwayDirection = (PreCommandLocation - ThreatLocation).GetSafeNormal();

	if (AwayDirection.IsNearlyZero())
	{
		AwayDirection = NPC->GetActorForwardVector();
	}

	FVector DesiredPoint = PreCommandLocation + AwayDirection * NPC->MoveAsideDistance;

	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSys)
		return;

	FNavLocation NavPoint;
	bool bFoundPoint = NavSys->ProjectPointToNavigation(
		DesiredPoint,
		NavPoint,
		FVector(NPC->MoveAsideDistance * 0.5f, NPC->MoveAsideDistance * 0.5f, NPC->MoveAsideDistance));

	if (!bFoundPoint)
	{
		// Couldn't find nav on that exact spot — settle for any reachable
		// point roughly in that direction instead.
		bFoundPoint = NavSys->GetRandomReachablePointInRadius(
			DesiredPoint,
			NPC->MoveAsideDistance * 0.5f,
			NavPoint);
	}

	if (!bFoundPoint)
		return;

	NPC->SetState(ENPCState::MoveAside);
	StopMovement();
	MoveToLocation(NavPoint.Location);
}

void ANPCAIController::HandleStuck()
{
	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());

	if (!NPC)
		return;

	// Whatever we were doing, falling back to wandering is always safe and
	// keeps the NPC from freezing in place indefinitely.
	GetWorld()->GetTimerManager().ClearTimer(WanderTimer);
	NPC->SetState(ENPCState::Wandering);
	MoveToRandomLocation();
}

void ANPCAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	if (!HasAuthority())
		return;

	ANPCCharacter* NPC = Cast<ANPCCharacter>(GetPawn());

	if (!NPC)
		return;

	switch (NPC->GetState())
	{
	case ENPCState::Wandering:
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
			// Path is clear. Drift back toward where we were standing
			// before the command interrupted us.
			NPC->SetState(ENPCState::Returning);
			MoveToLocation(PreCommandLocation);

			break;
		}

	case ENPCState::Returning:
		{
			// Back home (or as close as the navmesh allows) — resume
			// normal wandering behaviour.
			NPC->SetState(ENPCState::Wandering);
			MoveToRandomLocation();

			break;
		}

	default:
		break;
	}
}