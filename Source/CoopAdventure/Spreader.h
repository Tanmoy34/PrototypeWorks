// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spreader.generated.h"

UCLASS()
class COOPADVENTURE_API ASpreader : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ASpreader();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "mainmesh")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Root")
	USceneComponent* Root;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void PickpSprader();
};
