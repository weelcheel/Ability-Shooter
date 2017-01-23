#include "AbilityShooter.h"
#include "AbilityShooterGameState.h"
#include "UnrealNetwork.h"

AAbilityShooterGameState::AAbilityShooterGameState()
{
	scoreGoal = 1;
	bIsTeamGame = false;
}

void AAbilityShooterGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// everyone
	DOREPLIFETIME(AAbilityShooterGameState, scoreGoal);
	DOREPLIFETIME(AAbilityShooterGameState, bIsTeamGame);
	DOREPLIFETIME(AAbilityShooterGameState, numOfBots);
}