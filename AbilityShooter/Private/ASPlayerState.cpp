#include "AbilityShooter.h"
#include "ASPlayerState.h"
#include "UnrealNetwork.h"

AASPlayerState::AASPlayerState()
{

}

void AASPlayerState::SetRespawnTimer_Implementation(float repsawnTime)
{
	if (Role < ROLE_Authority)
		GetWorldTimerManager().SetTimer(respawnTimer, repsawnTime, false);
}

int32 AASPlayerState::GetTeamIndex() const
{
	return team;
}

void AASPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// everyone
	DOREPLIFETIME(AASPlayerState, team);
	DOREPLIFETIME(AASPlayerState, cash);
}