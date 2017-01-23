#include "AbilityShooter.h"
#include "AbilityShooterGameInstance.h"
#include "Json.h"
#include "Engine.h"
#include "AbilityShooterMainMenu.h"

const FString UAbilityShooterGameInstance::masterServerURL = "http://localhost:3308/";

UAbilityShooterGameInstance::UAbilityShooterGameInstance()
{
	http = &FHttpModule::Get();
}

void UAbilityShooterGameInstance::RequestUserProfile(const FString& userid)
{
	TSharedRef<IHttpRequest> request = http->CreateRequest();
	request->OnProcessRequestComplete().BindUObject(this, &UAbilityShooterGameInstance::OnProfileRequestProcessed);
	request->SetURL(masterServerURL + "userprofile"); //@DEBUG: change this at some point to the game website url
	request->SetVerb("GET");
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->SetHeader("userid", userid);
	request->ProcessRequest();
}

void UAbilityShooterGameInstance::OnProfileRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> jsonObject;
		TSharedRef<TJsonReader<> > reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//try to deserialize the json object
		if (FJsonSerializer::Deserialize(reader, jsonObject))
		{
			//Output it to the engine
			GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Green, "Received " + jsonObject->GetStringField("username") + "'s profile.");

			AAbilityShooterMainMenu* mm = Cast<AAbilityShooterMainMenu>(GetWorld()->GetAuthGameMode());
			if (IsValid(mm))
			{
				mm->ReceivedPlayerProfile(jsonObject);
			}
		}
	}
	else if (Response.IsValid() && Response->GetResponseCode() == 405)
	{
		//requested profile doesn't exist
		AAbilityShooterMainMenu* mm = Cast<AAbilityShooterMainMenu>(GetWorld()->GetAuthGameMode());
		if (IsValid(mm))
		{
			mm->RequestedPlayerProfileDoesntExist();
		}
	}
}

void UAbilityShooterGameInstance::RequestCreateNewProfile(const FString& userid, const FString& username, const FString& email)
{
	TSharedRef<IHttpRequest> request = http->CreateRequest();
	request->OnProcessRequestComplete().BindUObject(this, &UAbilityShooterGameInstance::OnProfileCreationRequestProcessed);
	request->SetURL(masterServerURL + "userprofile"); //@DEBUG: change this at some point to the game website url
	request->SetVerb("POST");
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->SetHeader("userid", userid);
	request->SetHeader("username", username);
	request->SetHeader("email", email);
	request->ProcessRequest();
}

void UAbilityShooterGameInstance::OnProfileCreationRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		AAbilityShooterMainMenu* mm = Cast<AAbilityShooterMainMenu>(GetWorld()->GetAuthGameMode());
		if (IsValid(mm))
		{
			mm->OnMakeHostNewProfileSucceeded();
		}
	}
}

void UAbilityShooterGameInstance::RequestPlayerServerList()
{
	TSharedRef<IHttpRequest> request = http->CreateRequest();
	request->OnProcessRequestComplete().BindUObject(this, &UAbilityShooterGameInstance::OnPlayerServersRequestProcessed);
	request->SetURL(masterServerURL + "playerservers"); //@DEBUG: change this at some point to the game website url
	request->SetVerb("GET");
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->ProcessRequest();
}

void UAbilityShooterGameInstance::OnPlayerServersRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		//TSharedPtr<FJsonObject> jsonObject;
		//TSharedRef<TJsonReader<> > reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Output it to the engine
		GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Green, "Received player server list.");

		AAbilityShooterMainMenu* mm = Cast<AAbilityShooterMainMenu>(GetWorld()->GetAuthGameMode());
		if (IsValid(mm))
		{
			mm->ReceivedPlayerServerList(Response->GetContentAsString());
		}

		//try to deserialize the json object
		/*if (FJsonSerializer::Deserialize(reader, jsonObject))
		{
			
		}*/
	}
}

void UAbilityShooterGameInstance::RequestToHostPlayerServer(const FString& gamemodePath, const FString& mapName, const FString& hostID, const FString& serverName)
{
	TSharedRef<IHttpRequest> request = http->CreateRequest();
	request->OnProcessRequestComplete().BindUObject(this, &UAbilityShooterGameInstance::OnHostPlayerServerRequestProcessed);
	request->SetURL(masterServerURL + "playerservers"); //@DEBUG: change this at some point to the game website url
	request->SetVerb("POST");
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->SetHeader("hostid", hostID);
	request->SetHeader("servername", serverName);
	request->ProcessRequest();

	playerHostGameModePath =  mapName + "?game=" + gamemodePath + "?listen";
}

void UAbilityShooterGameInstance::OnHostPlayerServerRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		//the master server accepted our request to be listed on the player servers so actually start the server on this instance
		if (IsValid(GetWorld()))
		{
			GetWorld()->ServerTravel(playerHostGameModePath);
		}
	}
}