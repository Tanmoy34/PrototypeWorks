// Fill out your copyright notice in the Description page of Project Settings.


#include "NPCCharacter.h"
#include "NPCAIController.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ANPCCharacter::ANPCCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	// Reasonable starting defaults so the feature works out of the box even
	// before a designer edits the list in the Details panel.
	FNPCVoiceCommand StandCommand;
	StandCommand.TriggerPhrases = { TEXT("stand"), TEXT("stay"), TEXT("wait here"), TEXT("stop") };
	StandCommand.Action = ENPCCommandAction::StandAndLook;
	VoiceCommands.Add(StandCommand);

	FNPCVoiceCommand WanderCommand;
	WanderCommand.TriggerPhrases = { TEXT("walk"), TEXT("wander"), TEXT("go"), TEXT("move around") };
	WanderCommand.Action = ENPCCommandAction::ResumeWander;
	VoiceCommands.Add(WanderCommand);

	FNPCVoiceCommand JumpCommand;
	JumpCommand.TriggerPhrases = { TEXT("jump"), TEXT("hop") };
	JumpCommand.Action = ENPCCommandAction::Jump;
	VoiceCommands.Add(JumpCommand);

	FNPCVoiceCommand FollowCommand;
	FollowCommand.TriggerPhrases = { TEXT("follow"), TEXT("follow me"), TEXT("come here"), TEXT("come") };
	FollowCommand.Action = ENPCCommandAction::Follow;
	VoiceCommands.Add(FollowCommand);
}

// Called when the game starts or when spawned
void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smoothly turn to face LookAtTarget while commanded to stand idle.
	// Runs on every machine (server + clients) purely for visuals; the
	// actual state/target are server-authoritative and replicated.
	if (CurrentState == ENPCState::Idle && LookAtTarget)
	{
		const FVector ToTarget = LookAtTarget->GetActorLocation() - GetActorLocation();
		FRotator DesiredRotation = ToTarget.Rotation();
		DesiredRotation.Pitch = 0.f;
		DesiredRotation.Roll = 0.f;

		const FRotator NewRotation = FMath::RInterpTo(
			GetActorRotation(),
			DesiredRotation,
			DeltaTime,
			LookAtTurnRate / 90.f);

		SetActorRotation(NewRotation);
	}
}

void ANPCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPCCharacter, CurrentState);
	DOREPLIFETIME(ANPCCharacter, LookAtTarget);
	DOREPLIFETIME(ANPCCharacter, FollowTarget);
}

void ANPCCharacter::OnRep_CurrentState()
{
	
}

void ANPCCharacter::OnRep_LookAtTarget()
{

}

void ANPCCharacter::OnRep_FollowTarget()
{

}

void ANPCCharacter::SetState(ENPCState newState)
{
	if (CurrentState == newState)return;

	CurrentState = newState;

	OnRep_CurrentState();
}

void ANPCCharacter::Server_CommandStandAndLook(AActor* Target)
{
	if (!HasAuthority()) return;

	if (!Target)
	{
		// Find the nearest player-controlled pawn to look at.
		float BestDistSq = TNumericLimits<float>::Max();
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (APawn* PlayerPawn = PC->GetPawn())
				{
					const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), GetActorLocation());
					if (DistSq < BestDistSq)
					{
						BestDistSq = DistSq;
						Target = PlayerPawn;
					}
				}
			}
		}
	}

	LookAtTarget = Target;
	OnRep_LookAtTarget();

	if (ANPCAIController* NPCController = Cast<ANPCAIController>(GetController()))
	{
		NPCController->CommandStandAndLook();
	}

	SetState(ENPCState::Idle);
}

void ANPCCharacter::Server_CommandResumeWander()
{
	if (!HasAuthority()) return;

	LookAtTarget = nullptr;
	OnRep_LookAtTarget();

	FollowTarget = nullptr;
	OnRep_FollowTarget();

	if (ANPCAIController* NPCController = Cast<ANPCAIController>(GetController()))
	{
		NPCController->CommandResumeWander();
	}
	else
	{
		SetState(ENPCState::wandering);
	}
}

void ANPCCharacter::Server_CommandJump()
{
	if (!HasAuthority()) return;

	// A momentary action - doesn't touch CurrentState, so the NPC just
	// hops in place (or mid-stride, if it happens to be wandering/following)
	// and carries on with whatever it was already doing.
	Jump();

	FTimerHandle JumpReleaseTimer;
	GetWorld()->GetTimerManager().SetTimer(
		JumpReleaseTimer,
		[this]() { StopJumping(); },
		0.2f,
		false);
}

void ANPCCharacter::Server_CommandFollow(AActor* Target)
{
	if (!HasAuthority()) return;

	LookAtTarget = nullptr;
	OnRep_LookAtTarget();

	FollowTarget = Target;
	OnRep_FollowTarget();

	if (ANPCAIController* NPCController = Cast<ANPCAIController>(GetController()))
	{
		NPCController->CommandFollow(Target);
	}

	SetState(ENPCState::Following);
}

bool ANPCCharacter::ProcessVoiceCommandText(const FString& RecognizedText, AActor* Speaker)
{
	if (!HasAuthority()) return false;

	if (RecognizedText.IsEmpty()) return false;

	const FString LowerText = RecognizedText.ToLower();

	for (const FNPCVoiceCommand& Command : VoiceCommands)
	{
		for (const FString& Phrase : Command.TriggerPhrases)
		{
			if (Phrase.IsEmpty()) continue;

			if (LowerText.Contains(Phrase.ToLower()))
			{
				switch (Command.Action)
				{
				case ENPCCommandAction::StandAndLook:
					Server_CommandStandAndLook(Speaker);
					break;
				case ENPCCommandAction::ResumeWander:
					Server_CommandResumeWander();
					break;
				case ENPCCommandAction::Jump:
					Server_CommandJump();
					break;
				case ENPCCommandAction::Follow:
					Server_CommandFollow(Speaker);
					break;
				}
				return true;
			}
		}
	}

	return false;
}

// Called to bind functionality to input
void ANPCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
