// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Car.generated.h"

class UBoxComponent;
class ACoopAdventureCharacter;
class UPrimitiveComponent;

UCLASS()
class COOPADVENTURE_API ACar : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACar();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Root")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mesh")
	USkeletalMeshComponent* CarMesh;

	// Overlap volumes placed over each door. Detects the player's equipped
	// Spreader touching either one. The default sizes/positions are just a
	// starting point - reposition them over the actual doors in the editor
	// or a BP subclass so they line up with your car mesh.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deformation", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* DoorCollisionLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deformation", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* DoorCollisionRight;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Server-side only: fires when something enters/leaves either door collision.
	UFUNCTION()
	void OnDoorBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDoorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// True if ArmCollision is still inside either door collision - used by
	// OnDoorEndOverlap so leaving one door box doesn't clear the flag while
	// the arm is still inside the other one.
	bool IsArmStillOverlappingAnyDoor(class USphereComponent* ArmCollision) const;

	// True once one of the player's equipped Spreader arm colliders
	// (ArmBottomCollision / ArmTopCollision on ACoopAdventureCharacter) is
	// inside a door collision. Replicated so every client can react to it.
	// Detection only for now - the deformation itself is the next step.
	UPROPERTY(ReplicatedUsing = OnRep_SpreaderTouchingDoor, BlueprintReadOnly, Category = "Deformation")
	bool bSpreaderTouchingDoor = false;

	UFUNCTION()
	void OnRep_SpreaderTouchingDoor();

	// Fired (server, and on every client via the RepNotify above) whenever
	// bSpreaderTouchingDoor changes. Empty for now - this is the hook point
	// for the actual deformation logic we're doing next.
	UFUNCTION(BlueprintImplementableEvent, Category = "Deformation")
	void OnSpreaderTouchingDoorChanged(bool bTouching);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// True if OtherComp is one of Character's Spreader arm collision
	// volumes, and Character actually has the Spreader equipped right now.
	bool IsSpreaderArmComponent(ACoopAdventureCharacter* Character, UPrimitiveComponent* OtherComp) const;

	// How many nearby bones to gather when a demeshing starts.
	UPROPERTY(EditAnywhere, Category = "Deformation")
	int32 NumBonesToAffect = 5;

	// The bones StartDemeshing found last time it ran - the next step will
	// apply transforms to these to actually deform the mesh.
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	TArray<FName> AffectedBones;

	// Returns the NumBones bones on CarMesh whose current world-space
	// location is closest to WorldLocation.
	TArray<FName> FindNearestBones(const FVector& WorldLocation, int32 NumBones) const;

	// Server-only. Called once when the spreader first touches a door:
	// announces it, then finds + logs the bones nearest ImpactLocation (the
	// door collision that was touched) so we know what the deformation step
	// should actually move.
	void StartDemeshing(const FVector& ImpactLocation);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
