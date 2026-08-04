// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCCharacter.generated.h"

UENUM(BlueprintType)
enum class ENPCState : uint8
{
	// Reused as the "commanded to stand still and look at the player" state.
	Idle,
	wandering,
	Waiting,
	MoveAside,
	Returning
};

// What a recognized voice command (or UI button) should make the NPC do.
UENUM(BlueprintType)
enum class ENPCCommandAction : uint8
{
	StandAndLook,
	ResumeWander
};

// One entry the designer can edit in the NPC's Details panel: a list of
// phrases that should all trigger the same action. Recognized speech text
// is matched against these with a case-insensitive "contains" check, so
// short, distinct phrases work best (e.g. "stand", "wait here", "stay").
USTRUCT(BlueprintType)
struct FNPCVoiceCommand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Command")
	TArray<FString> TriggerPhrases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Command")
	ENPCCommandAction Action = ENPCCommandAction::StandAndLook;
};

UCLASS()
class COOPADVENTURE_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANPCCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly)
	ENPCState CurrentState = ENPCState::wandering;

	UFUNCTION()
	void  OnRep_CurrentState();

	void SetState(ENPCState newState);

	ENPCState GetState() const
	{
		return CurrentState;
	}


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WanderRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MinWaitTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxWaitTime = 4.f;

	// -------- Commanded idle / look-at-player --------

	// Who the NPC should turn to face while in the commanded Idle state.
	// Replicated so it looks correct for every client, not just the host.
	UPROPERTY(ReplicatedUsing = OnRep_LookAtTarget, BlueprintReadOnly)
	AActor* LookAtTarget = nullptr;

	UFUNCTION()
	void OnRep_LookAtTarget();

	// How fast the NPC turns to face LookAtTarget, in degrees/sec.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Commands")
	float LookAtTurnRate = 180.f;

	// Server-authoritative: stop wandering, stand still, and face a player.
	// If Target is null, the nearest player-controlled pawn is used.
	void Server_CommandStandAndLook(AActor* Target = nullptr);

	// Server-authoritative: resume the normal wandering behavior.
	void Server_CommandResumeWander();

	// -------- Editable voice command list --------

	// Designer-editable list of phrases -> actions. Fill this in per-NPC
	// in the editor. Matching is case-insensitive "contains", so keep
	// phrases short and distinct from each other.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Command")
	TArray<FNPCVoiceCommand> VoiceCommands;

	// Server-only. Called by the player's Server RPC once speech has been
	// recognized as text. Finds the first matching command and executes it.
	// Returns true if a command matched.
	bool ProcessVoiceCommandText(const FString& RecognizedText, AActor* Speaker);

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
