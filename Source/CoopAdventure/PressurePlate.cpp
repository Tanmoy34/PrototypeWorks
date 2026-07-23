// Fill out your copyright notice in the Description page of Project Settings.


#include "PressurePlate.h"

// Sets default values
APressurePlate::APressurePlate()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	



	Activated =false;
	TriggerTag = "TriggerActor";



	//Creating Components
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);
	TriggerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TriggerMesh"));
	TriggerMesh->SetupAttachment(RootComp);
	TriggerMesh->SetIsReplicated(true);
	auto TriggerMeshAsset = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder"));
	if (TriggerMeshAsset.Succeeded())
    {
		TriggerMesh->SetStaticMesh(TriggerMeshAsset.Object);
    	TriggerMesh->SetRelativeScale3D(FVector(3.3f, 3.3f, 0.2f));
    	TriggerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
    }

	
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComp);
	Mesh->SetIsReplicated(true);
	auto MeshAsset = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("/Game/Stylized_Egypt/Meshes/building/SM_building_part_08.SM_building_part_08"));
	if (MeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(MeshAsset.Object);
		Mesh->SetRelativeScale3D(FVector(4.0f, 4.0f, 0.5f));
		Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 7.2f));
	}
}

// Called when the game starts or when spawned
void APressurePlate::BeginPlay()
{
	Super::BeginPlay();

	//Set replication
	SetReplicates(true);
	SetReplicateMovement(true);


	TriggerMesh->SetVisibility(false);
	TriggerMesh->SetCollisionProfileName(TEXT("OverlapAll"));
	
	
}

// Called every frame
void APressurePlate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		TArray<AActor*> OverlapActors;
		AActor* TriggerActor = 0;
        	TriggerMesh->GetOverlappingActors(OverlapActors);
        	for (int ActorIdx = 0; ActorIdx < OverlapActors.Num(); ActorIdx++)
        	{
        		AActor* A = OverlapActors[ActorIdx];
		        if (A->ActorHasTag(TriggerTag))
		        {
			        TriggerActor = A;
		        	break;
		        }
        
        		//GEngine->AddOnScreenDebugMessage(-1,1,FColor::Red,FString::Printf(TEXT("Name: %s"), *A->GetName()));
        	}
		if (TriggerActor)
		{
			if (!Activated)
			{
				Activated = true;
				//GEngine->AddOnScreenDebugMessage(-1,1,FColor::Red,TEXT("Activated"));
				OnActivate.Broadcast();
			}
		}
		else
		{
			if (Activated)
			{
				Activated = false;
				//GEngine->AddOnScreenDebugMessage(-1,1,FColor::Red,TEXT("Deactivated"));
				OnDeactivate.Broadcast();
			}
		}
	}
	
}

