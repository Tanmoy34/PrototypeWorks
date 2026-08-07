// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CoopAdventureCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class ASpreader;
class USkeletalMesh;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ACoopAdventureCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Shown once a Spreader has been picked up. Attached to the "HandGrip_L" socket on the character mesh, hidden until then. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HeldSpreaderMesh;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* EntractAction;

	/** Hold to talk. Bind this to the "V" key in the Input Action asset. */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* VoiceActivateAction;

public:

	/** Constructor */
	ACoopAdventureCharacter();	

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** V pressed: start listening for a voice command */
	void OnVoiceActivateStarted(const FInputActionValue& Value);

	/** V released: stop listening and process whatever was heard */
	void OnVoiceActivateCompleted(const FInputActionValue& Value);

public:
	
	
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Intract();

	// -------- NPC command UI buttons --------
	// Wire these directly to the OnClicked events of your two UMG buttons.

	/** Button: tell the (single) NPC to stand idle and look at this player. */
	UFUNCTION(BlueprintCallable, Category="NPC Commands")
	void CommandNPCStandIdle();

	/** Button: tell the NPC to resume roaming around. */
	UFUNCTION(BlueprintCallable, Category="NPC Commands")
	void CommandNPCResumeWander();

	/** Button: tell the NPC to jump once. */
	UFUNCTION(BlueprintCallable, Category="NPC Commands")
	void CommandNPCJump();

	/** Button: tell the NPC to start following this player. */
	UFUNCTION(BlueprintCallable, Category="NPC Commands")
	void CommandNPCFollow();

	/** Called by the voice component once speech has been turned into text. */
	UFUNCTION(BlueprintCallable, Category="Voice Command")
	void IssueNPCVoiceCommand(const FString& RecognizedText);

	// -------- Spreader pickup --------

	/** True once this character has picked up a Spreader. Replicated - read this from your AnimBP to drive a hold pose. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pickup")
	bool bSpreaderPicked = false;

	/** Server-only: gives this character the Spreader mesh in-hand and sets bSpreaderPicked. Called by ASpreader::PickpSprader once it has authoritatively decided this character picked it up - don't call this directly from client code. */
	void PickUpSpreaderMesh(USkeletalMesh* InMesh);

protected:

	UFUNCTION(Server, Reliable)
	void Server_CommandNPCStandIdle();

	UFUNCTION(Server, Reliable)
	void Server_CommandNPCResumeWander();

	UFUNCTION(Server, Reliable)
	void Server_CommandNPCJump();

	UFUNCTION(Server, Reliable)
	void Server_CommandNPCFollow();

	UFUNCTION(Server, Reliable)
	void Server_IssueNPCVoiceCommand(const FString& RecognizedText);

	// Below this speed (cm/s) the player counts as "standing still" for
	// the purpose of issuing NPC commands. Also blocked entirely while
	// airborne (jumping/falling).
	UPROPERTY(EditAnywhere, Category = "NPC Commands")
	float StandingStillVelocityThreshold = 10.f;

	// Server-side gate used by every NPC command entry point (both UI
	// buttons and voice): only the host may command the NPC, and only
	// while standing still. On a listen server, the host's own
	// PlayerController has no NetConnection (it's local); every remote
	// client's does - that's what "host" is checked against here. If you
	// ever move to a dedicated server, nobody satisfies that check, so
	// you'd need to swap in your own host/admin flag at that point.
	bool CanIssueNPCCommand(FString& OutReason) const;

	// Shows OutReason from a rejected command on the issuing client only.
	UFUNCTION(Client, Reliable)
	void Client_NotifyNPCCommandRejected(const FString& Reason);

	// Single-NPC helper: this project only ever has one NPC in the level,
	// so we just grab the first one that exists rather than tracking a
	// reference. Server-only.
	class ANPCCharacter* FindTheNPC() const;

	// -------- Spreader pickup (internals) --------

	// Mesh asset copied off the Spreader when picked up. Replicated so
	// HeldSpreaderMesh can show the right mesh on every client, independent
	// of bSpreaderPicked's own replication (don't rely on both arriving in
	// the same order - this RepNotify carries all the visual work).
	UPROPERTY(ReplicatedUsing = OnRep_PickedSpreaderMesh)
	USkeletalMesh* PickedSpreaderMeshAsset;

	UFUNCTION()
	void OnRep_PickedSpreaderMesh();

	// Routes a line-trace Intract pickup through the server, since Intract()
	// itself runs locally on whichever machine pressed the interact key, and
	// only the server may authoritatively destroy/replicate the Spreader.
	UFUNCTION(Server, Reliable)
	void Server_PickUpSpreader(ASpreader* TargetSpreader);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
