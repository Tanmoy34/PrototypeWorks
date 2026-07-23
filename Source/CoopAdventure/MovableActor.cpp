// Fill out your copyright notice in the Description page of Project Settings.


#include "MovableActor.h"


AMovableActor::AMovableActor()
{
 	
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>("RootComp");
	SetRootComponent(RootComp);

	Point1= CreateDefaultSubobject<UArrowComponent>("Point1");
	Point1->SetupAttachment(RootComp);
	Point1->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	
	Point2= CreateDefaultSubobject<UArrowComponent>("Point2");
	Point2->SetupAttachment(RootComp);
	Point2->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(RootComp);\
	Mesh->SetIsReplicated(true);

	Transpoter = CreateDefaultSubobject<UTranspoter>("Transporter");

	//Replication setup
	bReplicates = true;
	SetReplicateMovement(true);

}


void AMovableActor::BeginPlay()
{
	Super::BeginPlay();

	FVector StartPoint =  GetActorLocation() + Point1->GetRelativeLocation();
	FVector EndPoint =  GetActorLocation() + Point2->GetRelativeLocation();
	
	Transpoter->Setpoints(StartPoint, EndPoint);
}


void AMovableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

