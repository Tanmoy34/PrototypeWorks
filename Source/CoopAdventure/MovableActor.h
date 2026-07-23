// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Transpoter.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Actor.h"
#include "MovableActor.generated.h"

UCLASS()
class COOPADVENTURE_API AMovableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AMovableActor();

protected:
	
	virtual void BeginPlay() override;

public:	
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite,VisibleAnywhere)
	USceneComponent* RootComp;
	
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere)
	UArrowComponent* Point1;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere)
	UArrowComponent* Point2;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(BlueprintReadWrite,VisibleAnywhere)
	UTranspoter* Transpoter;
};
