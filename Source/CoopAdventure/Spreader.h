// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spreader.generated.h"

class UCapsuleComponent;
class ACoopAdventureCharacter;

UCLASS()
class COOPADVENTURE_API ASpreader : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ASpreader();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "mainmesh")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Root")
	USceneComponent* Root;

	// Overlap volume used to detect a player walking into the Spreader so it
	// can be auto-picked-up. Sized a bit larger than the mesh - tweak in BP
	// if it doesn't match your art.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CollisionCapsule;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Bound to CollisionCapsule's overlap. Fires on every machine's own
	// simulation of this actor, but only the server acts on it (see
	// PickpSprader) - clients just see the result replicate down.
	UFUNCTION()
	void OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Picks this Spreader up: gives PickingCharacter the mesh in its hand
	// (via PickingCharacter->PickUpSpreaderMesh) and destroys this actor.
	// Server-only, guarded internally with HasAuthority(). Called either
	// from OnCollisionBeginOverlap above, or from the player's line-trace
	// Intract, routed through ACoopAdventureCharacter::Server_PickUpSpreader
	// so it also works when a remote client is the one interacting.
	void PickpSprader(ACoopAdventureCharacter* PickingCharacter);
};
