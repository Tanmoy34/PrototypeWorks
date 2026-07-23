// Fill out your copyright notice in the Description page of Project Settings.


#include "Spreader.h"


// Sets default values
ASpreader::ASpreader()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;


	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(Root);
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

void ASpreader::PickpSprader()
{
	UE_LOG(LogTemp, Warning, TEXT("PickpSprader"));
}

