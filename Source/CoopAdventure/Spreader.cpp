// Fill out your copyright notice in the Description page of Project Settings.


#include "Spreader.h"
#include "Components/CapsuleComponent.h"
#include "CoopAdventureCharacter.h"


// Sets default values
ASpreader::ASpreader()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Needed so Destroy() below (called from PickpSprader) actually removes
	// this actor for every client too, not just on the server.
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;


	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh");
	Mesh->SetupAttachment(Root);
	// Pickup detection is handled by CollisionCapsule below - keep the
	// visual mesh non-colliding so it doesn't also block/overlap on its own
	// (skeletal mesh) collision.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>("CollisionCapsule");
	CollisionCapsule->SetupAttachment(Root);
	CollisionCapsule->InitCapsuleSize(40.f, 60.f);
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionCapsule->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionCapsule->SetGenerateOverlapEvents(true);
	CollisionCapsule->OnComponentBeginOverlap.AddDynamic(this, &ASpreader::OnCollisionBeginOverlap);
}

// Called when the game starts or when spawned
void ASpreader::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpreader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpreader::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only the server is allowed to decide "this just got picked up" -
	// otherwise every client would independently try to Destroy() their own
	// copy of this actor and you'd get desyncs/log spam.
	if (!HasAuthority())
	{
		return;
	}

	if (ACoopAdventureCharacter* Character = Cast<ACoopAdventureCharacter>(OtherActor))
	{
		PickpSprader(Character);
	}
}

void ASpreader::PickpSprader(ACoopAdventureCharacter* PickingCharacter)
{
	if (!HasAuthority() || !PickingCharacter)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PickpSprader: %s picked up %s"), *PickingCharacter->GetName(), *GetName());

	// Hand the mesh asset to the character - it replicates this to every
	// client itself, so we don't need to do anything special here for
	// clients to see it in-hand.
	PickingCharacter->PickUpSpreaderMesh(Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr);

	// Replicated actor + bReplicates=true above means this destruction
	// propagates to every client automatically.
	Destroy();
}

