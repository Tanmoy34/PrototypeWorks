// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"




#define EasyPrint(X) GEngine->AddOnScreenDebugMessage(-1,15,FColor::Green,X)

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
{
	CreateServerAfterDestroy =false;
	DestroyServerName="";
	ServerNameToFind = "";
	//Give Session A Name
	MySessionName = FName("Co-op Advencher Session");
}

void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
		EasyPrint(SubsystemName);

		SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this,&UMultiplayerSessionsSubsystem::OnCreatSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this,&UMultiplayerSessionsSubsystem::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this,&UMultiplayerSessionsSubsystem::FindSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this,&UMultiplayerSessionsSubsystem::OnJoinSessionComplete);
		}
	}
		
}

void UMultiplayerSessionsSubsystem::CreateSession(FString ServerName)
{
	//EasyPrint("Create session Called");
	
	//Checking Servername Should not be Empty
	if (ServerName.IsEmpty())
	{
		//EasyPrint("server name is empty");
		ServerCreateDel.Broadcast(false);
		return;
	}

	

	
	
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(MySessionName);
	if (ExistingSession)
	{
		//EasyPrint("Exsiting session Ask to Destroy");
		CreateServerAfterDestroy = true;
		DestroyServerName=ServerName;
		SessionInterface->DestroySession(MySessionName);
		return;
	}

	//Session Setting and Properties
	FOnlineSessionSettings SessionSettings;
	
	SessionSettings.bAllowJoinInProgress = true; //Can Other player Can Join after Session Inisiated
	SessionSettings.bIsDedicated = false; //says Server didiacted or not
	SessionSettings.bShouldAdvertise = true; // Allow Other Player To vissualize My session
	SessionSettings.NumPublicConnections = 2; //How many Player Can maximum join

	//Steam sestting
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;
	bool IsLan = false;
	if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
	{
		IsLan = true;
	}
	
	SessionSettings.bIsLANMatch = IsLan;

	//Saveing session Name and Use Later To join same Server name
	SessionSettings.Set(FName("SERVER_NAME"), ServerName,EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	
	SessionInterface->CreateSession(0,MySessionName,SessionSettings);
	
	




	
	
}

void UMultiplayerSessionsSubsystem::FindSession(FString ServerName)
{
	if(ServerName.IsEmpty())
	{
		EasyPrint("Servername Emty");
		ServerJoinDel.Broadcast(false);
		return;
		
	}
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	bool IsLan = false;
	if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
	{
		IsLan = true;
	}
	SessionSearch->bIsLanQuery = IsLan;
	SessionSearch-> MaxSearchResults = 9999;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	ServerNameToFind = ServerName;
	SessionInterface->FindSessions(0,SessionSearch.ToSharedRef());




	
}

void UMultiplayerSessionsSubsystem::OnCreatSessionComplete(FName SessionName, bool WasSucssessFull)
{
	//EasyPrint(FString::Printf(TEXT("ON session Creation completed: %d"),WasSucssessFull));

	ServerCreateDel.Broadcast(WasSucssessFull);
	
	if (WasSucssessFull)
	{
		
		FString Path = "/Game/ThirdPerson/Maps/ThirdPersonMap?listen";

		if (!GameMapPath.IsEmpty())
		{
			Path = FString::Printf(TEXT("%s?listen"), *GameMapPath);
		}

		GetWorld()->ServerTravel(Path);
		
	}
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool WasSucssessFull)
{
	FString msg = FString::Printf(TEXT("OnDestroySession completed: Session Name:%s,SucessFull: %d"),*SessionName.ToString() ,WasSucssessFull);
	EasyPrint(msg);
	if (CreateServerAfterDestroy)
	{
		CreateServerAfterDestroy = false;
		CreateSession(DestroyServerName);
	}
	
}

void UMultiplayerSessionsSubsystem::FindSessionComplete(bool WasSucssessFull)
{
	FString msg = FString::Printf(TEXT("OnDestroySession completed %d"),WasSucssessFull);
	EasyPrint(msg);

	if (!WasSucssessFull)
	{
		return;
	}
	if (ServerNameToFind.IsEmpty())
	{
		return;
	}
	TArray<FOnlineSessionSearchResult> Results =  SessionSearch->SearchResults;
	FOnlineSessionSearchResult* CurrectResult = 0;
	if (Results.Num() > 0)
	{
		FString massage = FString::Printf(TEXT("Session Found %d"),Results.Num());
		EasyPrint(massage);

		for ( FOnlineSessionSearchResult result : Results)
		{
			if (result.IsValid())
			{
				FString Servername = "No-Name";
				result.Session.SessionSettings.Get(FName("SERVER_NAME"),Servername);
				

				if (Servername.Equals(ServerNameToFind))
				{
					CurrectResult = &result;
					FString massage2 = FString::Printf(TEXT("Found erver Name :  %s"),*Servername);
					EasyPrint(massage2);
					break;
				}
			}
		}
		if (CurrectResult)
		{
			SessionInterface->JoinSession(0,MySessionName,*CurrectResult);
			 
		}
		else
		{
			EasyPrint("Couldn't Find Server Result");
			ServerJoinDel.Broadcast(false);
			ServerNameToFind ="";
		}
	}
	else
	{
		EasyPrint("Zero Session Found");
		ServerJoinDel.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	ServerJoinDel.Broadcast(Result == EOnJoinSessionCompleteResult::Success);
	if (Result==EOnJoinSessionCompleteResult::Success)
	{
		FString Adress = "";
		bool Sucsses =   SessionInterface->GetResolvedConnectString(MySessionName,Adress);
		if (Sucsses)
		{
			EasyPrint(FString::Printf(TEXT("Server adress To join : %s"),*Adress));
			APlayerController* PlayerController =  GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(Adress,TRAVEL_Absolute);
			}
		}
	}
	else
	{
		EasyPrint("Join Failed!!!!!");
	}
}

