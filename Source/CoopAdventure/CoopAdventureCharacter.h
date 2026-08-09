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
class ACar;

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

	/** Collision volumes on the held Spreader mesh's arms, for later hit/collision detection while it's equipped. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* ArmBottomCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* ArmTopCollision;

	/** First-person camera, attached to the head socket. Inactive until Deform Mode starts - only the host ever switches to it (see Server_EnterDeformMode). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCamera;

	/** Bone/socket on the character mesh FirstPersonCamera attaches to. Defaults to "head" - change this to match your skeleton if needed. */
	UPROPERTY(EditAnywhere, Category="Camera")
	FName HeadSocketName = TEXT("head");
	
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

	/** Toggles Deform Mode while touching a car door with the spreader. Bind this to the "Y" key in the Input Action asset. */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DeformActivateAction;

	/** Bends the affected car bones while in Deform Mode. Should be a 1D axis Input Action bound so "Q" gives a negative value and "E" gives a positive value in the Input Action asset - camera look stays on the mouse, this is what Q/E drive instead. */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BendAction;

	/** Exits Deform Mode and restores normal control. Bind this to the "R" key in the Input Action asset. */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ExitDeformAction;

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

	/** Y pressed: enter Deform Mode (only takes effect while touching a car door with the spreader equipped) */
	void OnDeformActivateStarted(const FInputActionValue& Value);

	/** R pressed: exit Deform Mode and restore normal control */
	void OnExitDeformActivateStarted(const FInputActionValue& Value);

	/** Q/E held: bends the car's affected bones while in Deform Mode. Ignored otherwise. */
	void OnBendInput(const FInputActionValue& Value);

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

	/** True if this character belongs to the host (listen server) rather than a remote client. A listen server's own PlayerController has no NetConnection (it's local); every remote client's does - that's what's checked here. Used to gate both NPC commands and Spreader pickup to the host only. */
	bool IsHostPlayer() const;

	/** True while Deform Mode is active. Replicated - walking, jumping, and voice command are all suppressed while this is set (see DoMove/DoJumpStart/DoJumpEnd/OnVoiceActivateStarted). Only reachable while touching a car door with the spreader equipped, which in practice restricts it to the host (only the host can ever pick up the spreader). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deformation")
	bool bIsDeforming = false;

	/** Called by ACar when this character's spreader-arm colliders enter/leave its door collision (server-only). Not meant to be called from anywhere else. Note: leaving the collision does NOT exit Deform Mode by itself - only Server_ExitDeformMode (the R key) does that, so control stays frozen until you explicitly exit. */
	void SetTouchingCar(ACar* InCar);
	void ClearTouchingCar(ACar* InCar);

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

	// -------- Deform mode (internals) --------

	// The car this character's spreader-arm colliders are currently inside
	// the door collision of, if any. Set/cleared by ACar - server-only
	// bookkeeping, never replicated (only the server's own overlap matters).
	UPROPERTY()
	ACar* TouchingCar = nullptr;

	// Server-side: turns Deform Mode on. Only succeeds while TouchingCar is
	// set (i.e. the spreader is actually on a door) - otherwise the request
	// is rejected and the requesting client is told why. Bound to Y.
	UFUNCTION(Server, Reliable)
	void Server_EnterDeformMode();

	// Server-side: turns Deform Mode off and restores normal movement/
	// camera. Bound to R. Safe to call even if not currently deforming.
	UFUNCTION(Server, Reliable)
	void Server_ExitDeformMode();

	// Generic "your request didn't go through" notice, shown only to the
	// client that made the request.
	UFUNCTION(Client, Reliable)
	void Client_NotifyActionRejected(const FString& Reason);

	// Locally-controlled-only: swaps between FollowCamera and
	// FirstPersonCamera based on the current value of bIsDeforming. Called
	// right after bIsDeforming changes in Server_EnterDeformMode/
	// Server_ExitDeformMode - for a listen server host this affects their
	// view immediately, since their own Character actor IS the
	// authoritative one running this code.
	void UpdateCameraForDeformMode();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns the held Spreader's bottom-arm collision subobject **/
	FORCEINLINE class USphereComponent* GetArmBottomCollision() const { return ArmBottomCollision; }

	/** Returns the held Spreader's top-arm collision subobject **/
	FORCEINLINE class USphereComponent* GetArmTopCollision() const { return ArmTopCollision; }
};
