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

	/** Called by the voice component once speech has been turned into text. */
	UFUNCTION(BlueprintCallable, Category="Voice Command")
	void IssueNPCVoiceCommand(const FString& RecognizedText);

protected:

	UFUNCTION(Server, Reliable)
	void Server_CommandNPCStandIdle();

	UFUNCTION(Server, Reliable)
	void Server_CommandNPCResumeWander();

	UFUNCTION(Server, Reliable)
	void Server_IssueNPCVoiceCommand(const FString& RecognizedText);

	// Single-NPC helper: this project only ever has one NPC in the level,
	// so we just grab the first one that exists rather than tracking a
	// reference. Server-only.
	class ANPCCharacter* FindTheNPC() const;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
