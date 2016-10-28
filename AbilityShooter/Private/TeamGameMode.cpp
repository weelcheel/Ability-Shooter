#include "AbilityShooter.h"
#include "TeamGameMode.h"
#include "ASPlayerState.h"
#include "ShooterSpawnPoint.h"

ATeamGameMode::ATeamGameMode()
{
	for (int32 i = 0; i < teamCount; i++)
		numTeamPlayers.Add(0);
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

	Super::PostLogin(newPlayer);

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
		}
	}
}

AActor* ATeamGameMode::FindBestShooterSpawn(class AController* player)
{
	TArray<FSpawnPointEntry> spawns;

	for (TObjectIterator<AShooterSpawnPoint> itr; itr; ++itr)
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

	//there are teams so return the spawn point with the highest score
	return spawns.Num() > 0 ? spawns[spawns.Num()-1].spawn : nullptr;
}