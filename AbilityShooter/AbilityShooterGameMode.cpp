// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AbilityShooter.h"
#include "AbilityShooterGameMode.h"
#include "AbilityShooterCharacter.h"
#include "AbilityShooterPlayerController.h"
#include "ASPlayerState.h"
#include "PlayerHUD.h"
#include "ShooterSpawnPoint.h"
#include "Camera/CameraActor.h"
#include "AbilityShooterGameState.h"
#include "AbilityAIController.h"

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
	GameStateClass = AAbilityShooterGameState::StaticClass();
	aiShooterBotClass = AAbilityShooterBot::StaticClass();

	basePlayerRespawnTime = 10.f;
	maxNumPlayers = 5;

	bDelayedStart = true;
}

void AAbilityShooterGameMode::PreLogin(const FString& Options, const FString& Address, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	if (NumPlayers + 1 > maxNumPlayers)
	{
		ErrorMessage = "serverfull";
		return;
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
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

void AAbilityShooterGameMode::RestartPlayer(class AController* NewPlayer)
{
	if (NewPlayer == NULL || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("RestartPlayer %s"), (NewPlayer && NewPlayer->PlayerState) ? *NewPlayer->PlayerState->PlayerName : TEXT("Unknown"));

	if (NewPlayer->PlayerState && NewPlayer->PlayerState->bOnlySpectator)
	{
		UE_LOG(LogTemp, Verbose, TEXT("RestartPlayer tried to restart a spectator-only player!"));
		return;
	}

	//AActor* StartSpot = FindPlayerStart(NewPlayer);
	AActor* StartSpot = FindBestShooterSpawn(NewPlayer);

	// if a start spot wasn't found,
	if (StartSpot == NULL)
	{
		// check for a previously assigned spot
		if (NewPlayer->StartSpot != NULL)
		{
			StartSpot = NewPlayer->StartSpot.Get();
			UE_LOG(LogTemp, Warning, TEXT("Player start not found, using last start spot"));
		}
		else
		{
			// otherwise abort
			UE_LOG(LogTemp, Warning, TEXT("Player start not found, failed to restart player"));
			return;
		}
	}
	// try to create a pawn to use of the default class for this player
	if (NewPlayer->GetPawn() == NULL && NewPlayer->GetClass()->IsChildOf(AAbilityAIController::StaticClass()))
	{
		AAbilityShooterBot* bot = GetWorld()->SpawnActor<AAbilityShooterBot>(aiShooterBotClass, StartSpot->GetActorLocation(), StartSpot->GetActorRotation());
		NewPlayer->SetPawn(bot);
	}
	else if (NewPlayer->GetPawn() == NULL && GetDefaultPawnClassForController(NewPlayer) != NULL)
	{
		NewPlayer->SetPawn(SpawnDefaultPawnFor(NewPlayer, StartSpot));
	}

	if (NewPlayer->GetPawn() == NULL)
	{
		NewPlayer->FailedToSpawnPawn();
	}
	else
	{
		// initialize and start it up
		InitStartSpot(StartSpot, NewPlayer);

		NewPlayer->StartSpot = StartSpot;

		// @todo: this was related to speedhack code, which is disabled.
		/*
		if ( NewPlayer->GetAPlayerController() )
		{
		NewPlayer->GetAPlayerController()->TimeMargin = -0.1f;
		}
		*/
		NewPlayer->Possess(NewPlayer->GetPawn());

		// If the Pawn is destroyed as part of possession we have to abort
		if (NewPlayer->GetPawn() == nullptr)
		{
			NewPlayer->FailedToSpawnPawn();
		}
		else
		{
			// set initial control rotation to player start's rotation
			NewPlayer->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation(), true);

			FRotator NewControllerRot = StartSpot->GetActorRotation();
			NewControllerRot.Roll = 0.f;
			NewPlayer->SetControlRotation(NewControllerRot);

			SetPlayerDefaults(NewPlayer->GetPawn());

			K2_OnRestartPlayer(NewPlayer);
		}
	}

#if !UE_WITH_PHYSICS
	if (NewPlayer->GetPawn() != NULL)
	{
		UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(NewPlayer->GetPawn()->GetMovementComponent());
		if (CharacterMovement)
		{
			CharacterMovement->bCheatFlying = true;
			CharacterMovement->SetMovementMode(MOVE_Flying);
		}
	}
#endif	//!UE_WITH_PHYSICS
}

AActor* AAbilityShooterGameMode::FindBestShooterSpawn(class AController* player)
{
	TArray<FSpawnPointEntry> spawns;

	for (TActorIterator<AShooterSpawnPoint> itr(GetWorld()); itr; ++itr)
	{
		AShooterSpawnPoint* spawn = (*itr);
		if (IsValid(spawn))
		{
			FSpawnPointEntry spawnEntry;
			spawnEntry.spawn = spawn;
			spawnEntry.score = spawn->GetSpawnScoreForPlayer(Cast<AASPlayerState>(player->PlayerState));

			spawns.Add(spawnEntry);
		}
	}

	spawns.Sort();

	//there are no teams so return the spawn point with the lowest score since every player will add score to the spawn point
	return spawns.Num() > 0 ? spawns[0].spawn : nullptr;
}

void AAbilityShooterGameMode::ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType, AShooterItem* killingItem)
{

	AAbilityShooterPlayerController* respawningPlayer = Cast<AAbilityShooterPlayerController>(KilledPlayer);
	if (IsValid(respawningPlayer))
		respawningPlayer->SetRespawnTimer(GetRespawnTime(respawningPlayer));

	//reward killer with cash
	if (IsValid(Killer))
	{
		AASPlayerState* killerPS = Cast<AASPlayerState>(Killer->PlayerState);
		if (IsValid(killerPS))
		{
			int32 reward = baseCashPerKill;
			ModifyKillReward(reward);

			killerPS->cash += reward;
		}
	}

	//@TODO: notify player states about Shooter deaths
	if (IsValid(GameState) && IsValid(Killer) && IsValid(KilledPlayer))
	{
		AASPlayerState* killerState = Cast<AASPlayerState>(Killer->PlayerState);
		AASPlayerState* victimState = Cast<AASPlayerState>(KilledPlayer->PlayerState);

		AASPlayerState* ps = IsValid(killerState) ? killerState : victimState;
		if (IsValid(ps))
			ps->BroadcastKillToHUD(killerState, killingItem, victimState);
	}
}

float AAbilityShooterGameMode::GetRespawnTime(AController* player) const
{
	//respawn timers grow as the game goes on, 1.5 seconds for each minute passed in-game
	//@TODO: players' abilities will need to modify this value

	return basePlayerRespawnTime + ((GetWorld()->TimeSeconds / 60.f) * 1.5f);
}

bool AAbilityShooterGameMode::CanDealDamage(AASPlayerState* damageInstigator, AASPlayerState* damagedPlayer) const
{
	return true;
}

void AAbilityShooterGameMode::ModifyKillReward(int32& cashReward)
{
	
}

void AAbilityShooterGameMode::StartMatch()
{
	Super::StartMatch();

	AAbilityShooterGameState* gs = Cast<AAbilityShooterGameState>(GameState);
	if (IsValid(gs))
	{
		int32 numBots = gs->numOfBots;
		for (int32 i = 0; i < numBots; i++)
		{
			AAbilityAIController* ai = GetWorld()->SpawnActor<AAbilityAIController>(AAbilityAIController::StaticClass());
			AASPlayerState* ps = Cast<AASPlayerState>(ai->PlayerState);
			if (!IsValid(ps))
			{
				ps = GetWorld()->SpawnActor<AASPlayerState>(AASPlayerState::StaticClass());
				ai->PlayerState = ps;
			}

			ps->SetPlayerName("Bot " + (i + 1));

			GameState->PlayerArray.AddUnique(ps);

			RestartPlayer(ai);
		}
	}
}

void AAbilityShooterGameMode::EndMatch()
{
	Super::EndMatch();

	//@TODO: generate end game stats
	DecideWinner();

	//clear all timers
	GetWorldTimerManager().ClearAllTimersForObject(this);

	//tell all players to depossess their pawns and set their view targets to the end game camera

	//find the endgame camera
	ACameraActor* camera = nullptr;
	for (TActorIterator<ACameraActor> camitr(GetWorld()); camitr; ++camitr)
	{
		camera = *camitr;
		for (int32 i = 0; i < camera->Tags.Num(); i++)
		{
			FName tag = TEXT("endgame");
			if (camera->Tags[i] == tag)
				break;

			camera = nullptr;
		}

		if (camera != nullptr)
			break;
	}

	for (TActorIterator<APlayerController> pcitr(GetWorld()); pcitr; ++pcitr)
	{
		//set this player's view target
		APlayerController* pc = *pcitr;

		if (camera != nullptr)
			pc->ClientSetViewTarget(camera);

		if (IsValid(pc->GetPawn()))
		{
			pc->GetPawn()->Destroy();
		}
	}
}

int32 AAbilityShooterGameMode::DecideWinner()
{
	return 0;
}