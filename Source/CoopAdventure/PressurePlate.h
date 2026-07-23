// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "PressurePlate.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPressurePlateOnActivate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPressurePlateOnDeactivate);

UCLASS()
class COOPADVENTURE_API APressurePlate : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APressurePlate();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	USceneComponent* RootComp;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UStaticMeshComponent* TriggerMesh;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	bool Activated;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FName TriggerTag;

	UPROPERTY(BlueprintAssignable)
	FPressurePlateOnActivate OnActivate;
	UPROPERTY(BlueprintAssignable)
	FPressurePlateOnDeactivate OnDeactivate;

	

 	

};
