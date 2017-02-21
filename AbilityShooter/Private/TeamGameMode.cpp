#include "AbilityShooter.h"
#include "TeamGameMode.h"
#include "ASPlayerState.h"
#include "ShooterSpawnPoint.h"
#include "EngineUtils.h"
#include "AbilityShooterGameState.h"

ATeamGameMode::ATeamGameMode()
{
	
}

void ATeamGameMode::BeginPlay()
{
	Super::BeginPlay();

	for (int32 i = 0; i < teamCount; i++)
		numTeamPlayers.Add(0);

	AAbilityShooterGameState* gs = Cast<AAbilityShooterGameState>(GameState);
	if (IsValid(gs))
		gs->bIsTeamGame = true;
}

bool ATeamGameMode::CanDealDamage(AASPlayerState* damageInstigator, AASPlayerState* damagedPlayer) const
{
	if (!IsValid(damageInstigator) || !IsValid(damagedPlayer))
		return false;

	return damageInstigator->GetTeamIndex() != damagedPlayer->GetTeamIndex();
}

void ATeamGameMode::PostLogin(APlayerController* newPlayer)
{
	if (!IsValid(newPlayer))
		return;

	AASPlayerState* ps = Cast<AASPlayerState>(newPlayer->PlayerState);
	if (IsValid(ps))
	{
		int32 teamIndex = -1;

		//@TODO: add logic to keep groups that entered the game together on the same team

		if (teamIndex < 0)
		{
			//no team assigned from group, so just put this player on the smallest team (or random if the teams are the same size)
			TArray<int32> smallestTeams;
			for (int32 i = 0; i < teamCount; i++)
			{
				if (smallestTeams.Num() <= 0)
					smallestTeams.Add(i);
				else if (numTeamPlayers[i] < numTeamPlayers[smallestTeams[0]])
				{
					smallestTeams.Empty();
					for (int32 j = 0; j < teamCount; j++)
					{
						if (numTeamPlayers[j] == numTeamPlayers[i])
							smallestTeams.Add(j);
					}
				}
			}

			//one smallest team
			if (smallestTeams.Num() == 1)
				teamIndex = smallestTeams[0];
			else if (smallestTeams.Num() > 1)
				teamIndex = smallestTeams[FMath::RandHelper(smallestTeams.Num() - 1)];
			
		}

		//error if the team index is still not assigned
		if (teamIndex < 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Team index failed to be assigned."));
			return;
		}

		ps->SetTeamIndex(teamIndex);
		numTeamPlayers[teamIndex]++;
	}

	Super::PostLogin(newPlayer);
}

AActor* ATeamGameMode::FindBestShooterSpawn(class AController* player)
{
	TArray<FSpawnPointEntry> spawns;

	for (TActorIterator<AShooterSpawnPoint> itr(GetWorld()); itr; ++itr)
	{
		AShooterSpawnPoint* spawn = (*itr);
		if (IsValid(spawn))
		{
			AASPlayerState* ps = Cast<AASPlayerState>(player->PlayerState);

			if (IsValid(ps) && spawn->teamIndex == ps->GetTeamIndex())
			{
				FSpawnPointEntry spawnEntry;
				spawnEntry.spawn = spawn;
				spawnEntry.score = spawn->GetSpawnScoreForPlayer(Cast<AASPlayerState>(player->PlayerState));

				spawns.Add(spawnEntry);
			}
		}
	}

	spawns.Sort();

	//there are no teams so return the spawn point with the lowest score since every player will add score to the spawn point
	return spawns.Num() > 0 ? spawns[spawns.Num() - 1].spawn : nullptr;
}

void ATeamGameMode::RestartPlayer(class AController* NewPlayer)
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
	AActor* StartSpot = NewPlayer->StartSpot == nullptr ? FindBestShooterSpawn(NewPlayer) : NewPlayer->StartSpot.Get();

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
	if (NewPlayer->GetPawn() == NULL && GetDefaultPawnClassForController(NewPlayer) != NULL)
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