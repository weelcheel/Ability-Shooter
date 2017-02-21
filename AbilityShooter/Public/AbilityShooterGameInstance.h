#pragma once

#include "Engine/GameInstance.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "AbilityShooterGameInstance.generated.h"

UCLASS()
class UAbilityShooterGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:

	/* http module */
	FHttpModule* http;

	/* handle incoming http responses */
	void OnProfileRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/* handle responses for profile creations */
	void OnProfileCreationRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/* handle responses for player server requests */
	void OnPlayerServersRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/* handle responses for hosting player server requests */
	void OnHostPlayerServerRequestProcessed(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

public:
	UAbilityShooterGameInstance();

	/* current url that the master server for online is hosted at */
	static const FString masterServerURL;

	/* string path to send as a command line argument to start a new listen server on this machine */
	FString playerHostGameModePath;

	/* takes a string userid and requests that the master server send back profile data for the specified user */
	UFUNCTION(BlueprintCallable, Category=Profile)
	void RequestUserProfile(const FString& userid);

	/* request the server make a new profile for a player */
	UFUNCTION(BlueprintCallable, Category = Profile)
	void RequestCreateNewProfile(const FString& userid, const FString& username, const FString& email);

	/* request the player server list from the master server */
	UFUNCTION(BlueprintCallable, Category = Servers)
	void RequestPlayerServerList();

	/* request to add a player server entry in the master server and then actually create the server on the local (client's) machine */
	UFUNCTION(BlueprintCallable, Category = Servers)
	void RequestToHostPlayerServer(UPARAM() const FString& gamemodePath, UPARAM() const FString& mapName, UPARAM() const FString& hostID, UPARAM() const FString& serverName);
};