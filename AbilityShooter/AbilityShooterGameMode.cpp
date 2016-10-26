// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AbilityShooter.h"
#include "AbilityShooterGameMode.h"
#include "AbilityShooterCharacter.h"
#include "AbilityShooterPlayerController.h"
#include "ASPlayerState.h"
#include "PlayerHUD.h"

AAbilityShooterGameMode::AAbilityShooterGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PlayerControllerClass = AAbilityShooterPlayerController::StaticClass();
	PlayerStateClass = AASPlayerState::StaticClass();
	HUDClass = APlayerHUD::StaticClass();

	basePlayerRespawnTime = 10.f;
}

void AAbilityShooterGameMode::PostLogin(APlayerController* newPlayer)
{
	Super::PostLogin(newPlayer);

	AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(newPlayer);

	if (IsValid(pc))
	{
		pc->storeItems = storeItems;
		
		AASPlayerState* ps = Cast<AASPlayerState>(pc->PlayerState);
		if (IsValid(ps))
			ps->cash = 5000;
	}
}

void AAbilityShooterGameMode::ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType)
{
	//@TODO: notify player states about Shooter deaths

	AAbilityShooterPlayerController* respawningPlayer = Cast<AAbilityShooterPlayerController>(KilledPlayer);
	if (IsValid(respawningPlayer))
		respawningPlayer->SetRespawnTimer(GetRespawnTime(respawningPlayer));
}

float AAbilityShooterGameMode::GetRespawnTime(AAbilityShooterPlayerController* player) const
{
	//respawn timers grow as the game goes on, 1.5 seconds for each minute passed in-game
	//@TODO: players' abilities will need to modify this value

	return basePlayerRespawnTime + ((GetWorld()->TimeSeconds / 60.f) * 1.5f);
}

bool AAbilityShooterGameMode::CanDealDamage(AASPlayerState* damageInstigator, AASPlayerState* damagedPlayer) const
{
	return true;
}
