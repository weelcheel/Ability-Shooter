#include "AbilityShooter.h"
#include "Deathmatch.h"
#include "ASPlayerState.h"
#include "AbilityShooterGameState.h"

ADeathmatch::ADeathmatch()
{
	scoreGoal = 15;
	killScore = 1;
	deathScore = 0;

	bFirstBloodRewarded = false;

	winner = nullptr;
}

void ADeathmatch::ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType, AShooterItem* killingItem)
{
	Super::ShooterKilled(Killer, KilledPlayer, KilledPawn, DamageType, killingItem);

	//reward killer with score
	if (IsValid(Killer))
	{
		AASPlayerState* killerPS = Cast<AASPlayerState>(Killer->PlayerState);
		if (IsValid(killerPS))
			killerPS->Score += killScore;
	}

	//penalize (or reward) the killed
	if (IsValid(KilledPlayer))
	{
		AASPlayerState* killedPS = Cast<AASPlayerState>(KilledPlayer->PlayerState);
		if (IsValid(killedPS))
			killedPS->Score += deathScore;
	}

	//check to see if there is a winner yet
	for (APlayerState* ps : GameState->PlayerArray)
	{
		if (IsValid(ps))
		{
			if (ps->Score >= scoreGoal)
			{
				//theres a winner, so end the match
				winner = ps->GetInstigatorController();

				EndMatch();
			}
		}
	}
}

void ADeathmatch::ModifyKillReward(int32& cashReward)
{
	if (!bFirstBloodRewarded)
	{
		cashReward *= 2;
		bFirstBloodRewarded = true;
	}
}

void ADeathmatch::StartMatch()
{
	Super::StartMatch();

	bFirstBloodRewarded = false;

	AAbilityShooterGameState* gs = Cast<AAbilityShooterGameState>(GameState);
	if (IsValid(gs))
	{
		gs->scoreGoal = scoreGoal;
	}
}