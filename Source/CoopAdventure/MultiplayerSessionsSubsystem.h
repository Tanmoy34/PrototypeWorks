// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerSessionsSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FServerCreateDelegate,bool, WasSuccesful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FServerJoinDelegate,bool, WasSuccesful);
/**
 * 
 */
UCLASS()
class COOPADVENTURE_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:


	//variable
	IOnlineSessionPtr SessionInterface;

	bool CreateServerAfterDestroy;

	FString DestroyServerName;

	TSharedPtr<FOnlineSessionSearch>  SessionSearch;

	FString ServerNameToFind;

	FName MySessionName;

	UPROPERTY(BlueprintAssignable)
	FServerCreateDelegate ServerCreateDel;

    UPROPERTY(BlueprintAssignable)
    FServerJoinDelegate ServerJoinDel;

	UPROPERTY(BlueprintReadWrite)
	FString GameMapPath;

	UMultiplayerSessionsSubsystem();
	//Fuction
	 void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UFUNCTION(BlueprintCallable)
	void CreateSession(FString ServerName);

	UFUNCTION(BlueprintCallable)
	void FindSession(FString ServerName);

	

	void OnCreatSessionComplete(FName SessionName,bool WasSucssessFull );
	
	void OnDestroySessionComplete(FName SessionName,bool WasSucssessFull );

	void FindSessionComplete(bool WasSucssessFull);

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	

	

	
};
