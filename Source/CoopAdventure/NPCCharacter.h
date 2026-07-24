// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h" 
#include "NPCCharacter.generated.h"

UENUM(BlueprintType)
enum class ENPCState : uint8
{
	Idle,
	Wandering,
	Waiting,
	MoveAside,
	Returning
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly)
	ENPCState CurrentState = ENPCState::Wandering;


	UFUNCTION()
	void OnRep_CurrentState();
	
	void SetState(ENPCState NewState);

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


	

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
