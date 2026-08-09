// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Car.generated.h"

class UBoxComponent;
class ACoopAdventureCharacter;
class UPrimitiveComponent;
class UPoseableMeshComponent;

UCLASS()
class COOPADVENTURE_API ACar : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACar();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Root")
	USceneComponent* Root;

	// NOTE: this is a UPoseableMeshComponent, not a USkeletalMeshComponent.
	// Regular skeletal mesh components won't let you set an individual
	// bone's transform at runtime (animation just overwrites it every
	// tick) - PoseableMeshComponent is the engine-supported way to pose
	// bones by hand, which is what the bending below needs. It still reads
	// the same USkeletalMesh asset, but it does NOT run an AnimBP, so if
	// CarMesh ever needs its own animation later, that will need solving
	// separately.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mesh")
	UPoseableMeshComponent* CarMesh;

	// Overlap volumes placed over each door. Detects the player's equipped
	// Spreader touching either one. The default sizes/positions are just a
	// starting point - reposition them over the actual doors in the editor
	// or a BP subclass so they line up with your car mesh.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deformation", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* DoorCollisionLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deformation", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* DoorCollisionRight;

	// How strongly each tick of Q/E input bends the affected bones while
	// in Deform Mode. Tune this in the inspector to taste.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deformation")
	float BendingMagnitude = 2.f;

	// How quickly bending falls off across the affected bones. The
	// nearest bone (index 0 in AffectedBones) always bends at full
	// strength; each bone after that bends at BendFalloff times the
	// previous one's strength (e.g. 0.6 -> 1.0, 0.6, 0.36, 0.22, 0.13 for
	// 5 bones), so the impact point visibly bends the most and it tapers
	// off toward the edge of the affected area. 1.0 = no falloff, all
	// affected bones bend equally.
	UPROPERTY(EditAnywhere, Category = "Deformation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BendFalloff = 0.6f;

	// Safety clamp (world units) on how far any single bone's TOTAL
	// accumulated bend offset can drift from its rest position, so
	// holding Q/E for a long time can't stretch the mesh apart
	// indefinitely. 0 = no clamp.
	UPROPERTY(EditAnywhere, Category = "Deformation")
	float MaxBendDistance = 40.f;

	// Server-only. Called every tick Q or E is held while in Deform Mode
	// (see ACoopAdventureCharacter::OnBendInput). BendInput is negative for
	// Q, positive for E. Pushes each currently-affected bone along the
	// car's own right axis, scaled by BendingMagnitude and that bone's
	// falloff weight (see BendFalloff) so the bone nearest the impact
	// point bends the most and it tapers off from there.
	UFUNCTION(BlueprintCallable, Category = "Deformation")
	void ApplyDeformBend(float BendInput);

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

	// The bones StartDemeshing found last time it ran - these are what
	// ApplyDeformBend actually moves.
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	TArray<FName> AffectedBones;

	// World-space bone location captured once, right when a bone becomes
	// affected (in StartDemeshing). Bending is applied as an offset from
	// this rest location rather than the bone's current location, so
	// repeated Q/E presses accumulate predictably instead of compounding
	// off wherever the bone happened to already be.
	TMap<FName, FVector> BoneRestLocations;

	// How strongly each affected bone bends relative to the others, in
	// [0,1]. Set once in StartDemeshing from BendFalloff and each bone's
	// rank in AffectedBones (nearest = 1.0, tapering off from there).
	TMap<FName, float> BoneBendWeights;

	// Total accumulated bend offset applied to each affected bone so far,
	// in world space. Reset whenever a new demeshing starts.
	TMap<FName, FVector> BoneBendOffsets;

	// Server calls this (from ApplyDeformBend) with the freshly-computed
	// bone locations; runs on every machine (including the server itself)
	// so the bend is actually visible to everyone, not just the host doing
	// the bending. Unreliable since this can fire every tick Q/E is held
	// and a dropped update just gets overwritten by the next one.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ApplyBoneBend(const TArray<FName>& BoneNames, const TArray<FVector>& BoneLocations);

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
