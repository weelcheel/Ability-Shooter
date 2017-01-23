#include "AbilityShooter.h"
#include "AbilityShooterMainMenu.h"
#include "AbilityShooterGameInstance.h"
#include "Engine.h"
#include "ASPlayerState.h"
#include "AbilityShooterPlayerController.h"
#include "JsonObjectConverter.h"

AAbilityShooterMainMenu::AAbilityShooterMainMenu()
{
	currentMenuState = MS_ProfileNotSet;

	PlayerStateClass = AASPlayerState::StaticClass();
}

uint8 AAbilityShooterMainMenu::GetCurrentMenuState()
{
	return currentMenuState;
}

void AAbilityShooterMainMenu::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//if this is the owning player logging in for the first time
	if (NewPlayer->IsLocalPlayerController() && currentMenuState == MS_ProfileNotSet)
	{
		//@TODO: notify the UI that the profile hasn't downloaded yet to prevent the menu from showing
		playerHost = NewPlayer;

		//get the players profile info
		UAbilityShooterGameInstance* gi = Cast<UAbilityShooterGameInstance>(GetGameInstance());
		if (gi)
		{
			gi->RequestUserProfile(NewPlayer->PlayerState->UniqueId->ToString());
		}
	}
}

void AAbilityShooterMainMenu::ReceivedPlayerProfile(TSharedPtr<FJsonObject> profileJson)
{
	if (currentMenuState != MS_ProfileSet && playerHost && playerHost->PlayerState->UniqueId->ToString() == profileJson->GetStringField("uid"))
	{
		//check to see if the profile needs to be set up
		if (!profileJson->GetBoolField("isProfileSetup"))
		{
			currentMenuState = MS_ProfileNeedsCreation;

			MakeHostSetupNewProfile();
			return;
		}

		//this profile is already complete, so change the menu state and read the profile settings
		currentMenuState = MS_ProfileSet;

		//apply the correct player name
		playerHost->PlayerState->SetPlayerName(profileJson->GetStringField("username"));

		//@TODO: apply the player's saved settings

		//save the profile to the player state
		AASPlayerState* ps = Cast<AASPlayerState>(playerHost->PlayerState);
		if (IsValid(ps))
		{
			FString profileString;
			TSharedRef< TJsonWriter<> > writer = TJsonWriterFactory<>::Create(&profileString);
			FJsonSerializer::Serialize(profileJson.ToSharedRef(), writer);

			ps->profileJson = profileString;
		}

		//finally tell the player to show the main menu
		AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(playerHost);
		if (IsValid(pc))
		{
			pc->ShowMainMenuAfterProfileSet();
		}
	}
	else
	{
		//received another player's profile (could be a connected(ing) client or 
	}
}

void AAbilityShooterMainMenu::RequestedPlayerProfileDoesntExist()
{
	if (currentMenuState != MS_ProfileSet)
	{
		//this means that the host does not have an existing profile, so try to create one
		currentMenuState = MS_ProfileNeedsCreation;

		MakeHostSetupNewProfile();
	}
	else
	{
		//this game instance requested a profile that doesn't exist
		GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Green, "Requested an invalid profile.");
	}
}

void AAbilityShooterMainMenu::MakeHostSetupNewProfile()
{
	GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Green, "This player needs to setup a new profile.");

	AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(playerHost);
	if (IsValid(pc))
	{
		pc->ShowNewProfileUI();
	}
}

void AAbilityShooterMainMenu::OnMakeHostNewProfileSucceeded()
{
	GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Green, "Sucessfully created a new profile!");

	//get the new players profile info
	UAbilityShooterGameInstance* gi = Cast<UAbilityShooterGameInstance>(GetGameInstance());
	if (gi && playerHost)
	{
		gi->RequestUserProfile(playerHost->PlayerState->UniqueId->ToString());
	}
}

void AAbilityShooterMainMenu::ReceivedPlayerServerList(const FString& playerListJson)
{
	TArray<FPlayerServerInfo> serverList;
	FJsonObjectConverter::JsonArrayStringToUStruct(playerListJson, &serverList, 0, 0);

	if (playerHost)
	{
		AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(playerHost);
		if (pc)
		{
			pc->PopulatePlayerServerList(serverList);
		}
	}
}