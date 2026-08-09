// Fill out your copyright notice in the Description page of Project Settings.


#include "Car.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoopAdventureCharacter.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ACar::ACar()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Needed so bSpreaderTouchingDoor (and whatever deformation state we add
	// next) actually reaches clients.
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;

	CarMesh = CreateDefaultSubobject<USkeletalMeshComponent>("CarMesh");
	CarMesh->SetupAttachment(Root);

	// Two doors, two collisions. Placeholder sizes/offsets, mirrored left vs
	// right - move and resize both over the actual doors in the editor (or
	// in a BP subclass) once you see them against your mesh.
	DoorCollisionLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorCollisionLeft"));
	DoorCollisionLeft->SetupAttachment(CarMesh);
	DoorCollisionLeft->SetBoxExtent(FVector(15.f, 40.f, 50.f));
	DoorCollisionLeft->SetRelativeLocation(FVector(0.f, -90.f, 0.f));
	DoorCollisionLeft->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorCollisionLeft->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	DoorCollisionLeft->SetGenerateOverlapEvents(true);
	DoorCollisionLeft->OnComponentBeginOverlap.AddDynamic(this, &ACar::OnDoorBeginOverlap);
	DoorCollisionLeft->OnComponentEndOverlap.AddDynamic(this, &ACar::OnDoorEndOverlap);

	DoorCollisionRight = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorCollisionRight"));
	DoorCollisionRight->SetupAttachment(CarMesh);
	DoorCollisionRight->SetBoxExtent(FVector(15.f, 40.f, 50.f));
	DoorCollisionRight->SetRelativeLocation(FVector(0.f, 90.f, 0.f));
	DoorCollisionRight->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorCollisionRight->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	DoorCollisionRight->SetGenerateOverlapEvents(true);
	DoorCollisionRight->OnComponentBeginOverlap.AddDynamic(this, &ACar::OnDoorBeginOverlap);
	DoorCollisionRight->OnComponentEndOverlap.AddDynamic(this, &ACar::OnDoorEndOverlap);
}

// Called when the game starts or when spawned
void ACar::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ACar::IsSpreaderArmComponent(ACoopAdventureCharacter* Character, UPrimitiveComponent* OtherComp) const
{
	if (!Character || !OtherComp || !Character->bSpreaderPicked)
	{
		return false;
	}

	return OtherComp == Character->GetArmBottomCollision() || OtherComp == Character->GetArmTopCollision();
}

bool ACar::IsArmStillOverlappingAnyDoor(USphereComponent* ArmCollision) const
{
	if (!ArmCollision)
	{
		return false;
	}

	return (DoorCollisionLeft && DoorCollisionLeft->IsOverlappingComponent(ArmCollision))
		|| (DoorCollisionRight && DoorCollisionRight->IsOverlappingComponent(ArmCollision));
}

void ACar::OnDoorBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only the server decides whether a door is "being spread" - clients
	// just react to bSpreaderTouchingDoor replicating down.
	if (!HasAuthority())
	{
		return;
	}

	ACoopAdventureCharacter* Character = Cast<ACoopAdventureCharacter>(OtherActor);
	if (!IsSpreaderArmComponent(Character, OtherComp))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Car: spreader arm (%s) entered %s on %s"), *OtherComp->GetName(), *OverlappedComponent->GetName(), *GetName());

	Character->SetTouchingCar(this);

	if (!bSpreaderTouchingDoor)
	{
		bSpreaderTouchingDoor = true;
		OnRep_SpreaderTouchingDoor();
		StartDemeshing(OverlappedComponent->GetComponentLocation());
	}
}

void ACar::OnDoorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	ACoopAdventureCharacter* Character = Cast<ACoopAdventureCharacter>(OtherActor);
	if (!IsSpreaderArmComponent(Character, OtherComp))
	{
		return;
	}

	// Only clear once BOTH arms have left BOTH door collisions - otherwise
	// leaving one door box (or pulling one arm out) while still inside the
	// other would flicker the flag off/on.
	const bool bBottomStillIn = IsArmStillOverlappingAnyDoor(Character->GetArmBottomCollision());
	const bool bTopStillIn = IsArmStillOverlappingAnyDoor(Character->GetArmTopCollision());

	if (!bBottomStillIn && !bTopStillIn)
	{
		if (bSpreaderTouchingDoor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Car: spreader arms left door collision on %s"), *GetName());

			bSpreaderTouchingDoor = false;
			OnRep_SpreaderTouchingDoor();
		}

		Character->ClearTouchingCar(this);
	}
}

void ACar::OnRep_SpreaderTouchingDoor()
{
	// Server calls this directly too (RepNotifies don't fire locally when
	// the server sets the property itself) so both host and clients funnel
	// through the same hook point for whatever comes next.
	OnSpreaderTouchingDoorChanged(bSpreaderTouchingDoor);
}

void ACar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACar, bSpreaderTouchingDoor);
}

void ACar::StartDemeshing(const FVector& ImpactLocation)
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Car demeshing started on %s"), *GetName());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Car demeshing started"));
	}

	AffectedBones = FindNearestBones(ImpactLocation, NumBonesToAffect);

	UE_LOG(LogTemp, Warning, TEXT("Car demeshing: found %d nearby bone(s)"), AffectedBones.Num());
	for (const FName& BoneName : AffectedBones)
	{
		UE_LOG(LogTemp, Warning, TEXT("Car demeshing: affected bone -> %s"), *BoneName.ToString());
	}
}

TArray<FName> ACar::FindNearestBones(const FVector& WorldLocation, int32 NumBones) const
{
	TArray<FName> Result;

	if (!CarMesh)
	{
		return Result;
	}

	const int32 BoneCount = CarMesh->GetNumBones();
	if (BoneCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Car::FindNearestBones: CarMesh has no bones (no skeletal mesh assigned?)"));
		return Result;
	}

	// Gather every bone with its squared distance to WorldLocation, then
	// keep the closest NumBones.
	TArray<TPair<FName, float>> BoneDistances;
	BoneDistances.Reserve(BoneCount);

	for (int32 i = 0; i < BoneCount; ++i)
	{
		const FName BoneName = CarMesh->GetBoneName(i);
		const FVector BoneLocation = CarMesh->GetBoneLocation(BoneName, EBoneSpaces::WorldSpace);
		const float DistSq = FVector::DistSquared(BoneLocation, WorldLocation);

		BoneDistances.Add(TPair<FName, float>(BoneName, DistSq));
	}

	BoneDistances.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B)
	{
		return A.Value < B.Value;
	});

	const int32 Count = FMath::Min(NumBones, BoneDistances.Num());
	Result.Reserve(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		Result.Add(BoneDistances[i].Key);
	}

	return Result;
}
