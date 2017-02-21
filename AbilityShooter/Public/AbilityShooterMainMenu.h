#pragma once

#include "GameFramework/GameMode.h"
#include "Json.h"
#include "AbilityShooterMainMenu.generated.h"

/*enum for determining what state this player's main menu is in */
UENUM()
enum MainMenuState
{
	MS_ProfileNotSet,
	MS_ProfileNeedsCreation,
	MS_ProfileSet
};

/* struct to hold info about player server listings */
USTRUCT(Blueprintable)
struct FPlayerServerInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PlayerServer)
	FString hostUID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PlayerServer)
	FString serverName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PlayerServer)
	FString serverURL;
};

UCLASS()
class AAbilityShooterMainMenu : public AGameMode
{
	GENERATED_BODY()

protected:

	/* hosting player */
	AController* playerHost;

	MainMenuState currentMenuState;

public:
	AAbilityShooterMainMenu();

	/* gets the current state of the menu */
	UFUNCTION(BlueprintCallable, Category = MenuState)
	uint8 GetCurrentMenuState();

	/* post login function */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/* received a profile from the server */
	void ReceivedPlayerProfile(TSharedPtr<FJsonObject> profileJson);

	/* a requested profile from this game instance does not exist */
	void RequestedPlayerProfileDoesntExist();

	/* setup a new player's profile */
	void MakeHostSetupNewProfile();

	/* called whenever this player's request for a new profile is accepted */
	void OnMakeHostNewProfileSucceeded();

	/* called whenever the player's request for a server list is processed */
	void ReceivedPlayerServerList(const FString& playerListJson);
};