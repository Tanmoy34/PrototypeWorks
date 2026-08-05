// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoopAdventureCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "CoopAdventure.h"
#include "Spreader.h"
#include "NPCCharacter.h"
#include "VoiceCommandComponent.h"
#include "Kismet/GameplayStatics.h"

ACoopAdventureCharacter::ACoopAdventureCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ACoopAdventureCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACoopAdventureCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACoopAdventureCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACoopAdventureCharacter::Look);

		EnhancedInputComponent->BindAction(EntractAction, ETriggerEvent::Started, this, &ACoopAdventureCharacter::Intract);

		// Voice: hold V to talk, release to send the command off for recognition.
		EnhancedInputComponent->BindAction(VoiceActivateAction, ETriggerEvent::Started, this, &ACoopAdventureCharacter::OnVoiceActivateStarted);
		EnhancedInputComponent->BindAction(VoiceActivateAction, ETriggerEvent::Completed, this, &ACoopAdventureCharacter::OnVoiceActivateCompleted);
	}
	else
	{
		UE_LOG(LogCoopAdventure, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACoopAdventureCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACoopAdventureCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACoopAdventureCharacter::OnVoiceActivateStarted(const FInputActionValue& Value)
{
	if (!IsLocallyControlled()) return;

	if (UVoiceCommandComponent* VoiceComp = FindComponentByClass<UVoiceCommandComponent>())
	{
		VoiceComp->StartListening();
	}
}

void ACoopAdventureCharacter::OnVoiceActivateCompleted(const FInputActionValue& Value)
{
	if (!IsLocallyControlled()) return;

	if (UVoiceCommandComponent* VoiceComp = FindComponentByClass<UVoiceCommandComponent>())
	{
		VoiceComp->StopListening();
	}
}

void ACoopAdventureCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ACoopAdventureCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACoopAdventureCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ACoopAdventureCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ACoopAdventureCharacter::Intract()
{
	FVector Start = FollowCamera->GetComponentLocation();
	FVector End = Start + FollowCamera->GetForwardVector() * 1000.0f;

	FHitResult Hit;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		bHit ? FColor::Green : FColor::Red,
		false,
		2.f,
		0,
		2.f
	);

	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit: %s"), *Hit.GetActor()->GetName());
		ASpreader* Spreader = Cast<ASpreader>(Hit.GetActor());

		if (Spreader)
		{
			Spreader->PickpSprader();
		}
	}

	
}

// -------- NPC commands --------

void ACoopAdventureCharacter::CommandNPCStandIdle()
{
	Server_CommandNPCStandIdle();
}

void ACoopAdventureCharacter::CommandNPCResumeWander()
{
	Server_CommandNPCResumeWander();
}

void ACoopAdventureCharacter::CommandNPCJump()
{
	Server_CommandNPCJump();
}

void ACoopAdventureCharacter::CommandNPCFollow()
{
	Server_CommandNPCFollow();
}

void ACoopAdventureCharacter::IssueNPCVoiceCommand(const FString& RecognizedText)
{
	if (RecognizedText.IsEmpty()) return;

	Server_IssueNPCVoiceCommand(RecognizedText);
}

bool ACoopAdventureCharacter::CanIssueNPCCommand(FString& OutReason) const
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	// Host check: on a listen server the host's own PlayerController has
	// no NetConnection (it's local to this process); every remote
	// client's PlayerController does.
	if (!PC || PC->GetNetConnection() != nullptr)
	{
		OutReason = TEXT("Only the host can command the NPC.");
		return false;
	}

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		OutReason = TEXT("Land and stand still to command the NPC.");
		return false;
	}

	if (GetVelocity().SizeSquared() > FMath::Square(StandingStillVelocityThreshold))
	{
		OutReason = TEXT("Stand still to command the NPC.");
		return false;
	}

	return true;
}

void ACoopAdventureCharacter::Client_NotifyNPCCommandRejected_Implementation(const FString& Reason)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9003, 2.f, FColor::Orange, Reason);
	}
}

void ACoopAdventureCharacter::Server_CommandNPCStandIdle_Implementation()
{
	FString Reason;
	if (!CanIssueNPCCommand(Reason))
	{
		Client_NotifyNPCCommandRejected(Reason);
		return;
	}

	if (ANPCCharacter* NPC = FindTheNPC())
	{
		NPC->Server_CommandStandAndLook(this);
	}
}

void ACoopAdventureCharacter::Server_CommandNPCResumeWander_Implementation()
{
	FString Reason;
	if (!CanIssueNPCCommand(Reason))
	{
		Client_NotifyNPCCommandRejected(Reason);
		return;
	}

	if (ANPCCharacter* NPC = FindTheNPC())
	{
		NPC->Server_CommandResumeWander();
	}
}

void ACoopAdventureCharacter::Server_CommandNPCJump_Implementation()
{
	FString Reason;
	if (!CanIssueNPCCommand(Reason))
	{
		Client_NotifyNPCCommandRejected(Reason);
		return;
	}

	if (ANPCCharacter* NPC = FindTheNPC())
	{
		NPC->Server_CommandJump();
	}
}

void ACoopAdventureCharacter::Server_CommandNPCFollow_Implementation()
{
	FString Reason;
	if (!CanIssueNPCCommand(Reason))
	{
		Client_NotifyNPCCommandRejected(Reason);
		return;
	}

	if (ANPCCharacter* NPC = FindTheNPC())
	{
		NPC->Server_CommandFollow(this);
	}
}

void ACoopAdventureCharacter::Server_IssueNPCVoiceCommand_Implementation(const FString& RecognizedText)
{
	FString Reason;
	if (!CanIssueNPCCommand(Reason))
	{
		Client_NotifyNPCCommandRejected(Reason);
		return;
	}

	if (ANPCCharacter* NPC = FindTheNPC())
	{
		NPC->ProcessVoiceCommandText(RecognizedText, this);
	}
}

ANPCCharacter* ACoopAdventureCharacter::FindTheNPC() const
{
	// Single-NPC project: there's only ever one in the level, so just grab it.
	TArray<AActor*> FoundNPCs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANPCCharacter::StaticClass(), FoundNPCs);

	return FoundNPCs.Num() > 0 ? Cast<ANPCCharacter>(FoundNPCs[0]) : nullptr;
}
