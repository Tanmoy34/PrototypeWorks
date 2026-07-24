// Fill out your copyright notice in the Description page of Project Settings.


#include "NPCCharacter.h"
#include "NPCAIController.h"

#include "Net/UnrealNetwork.h" 


// Sets default values
ANPCCharacter::ANPCCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
}

// Called when the game starts or when spawned
void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ANPCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPCCharacter, CurrentState);
}

// Called every frame
void ANPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANPCCharacter::OnRep_CurrentState()
{
	
}

void ANPCCharacter::SetState(ENPCState NewState)
{
	if (CurrentState == NewState)
		return;

	CurrentState = NewState;

	OnRep_CurrentState();
}

void ANPCCharacter::HandleVoiceCommand(const FString& Command, const FVector& SourceLocation)
{
	// This is gameplay logic that changes movement/state, so it must only
	// ever run with authority (the Host/Server), same as everything else
	// driving this NPC.
	if (!HasAuthority())
		return;

	if (FVector::Dist(GetActorLocation(), SourceLocation) > ListenRadius)
		return;

	ANPCAIController* AICon = Cast<ANPCAIController>(GetController());

	if (!AICon)
		return;

	AICon->MoveAsideFrom(SourceLocation);
}

// Called to bind functionality to input
void ANPCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}