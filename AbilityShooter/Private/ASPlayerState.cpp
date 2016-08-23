#include "AbilityShooter.h"
#include "ASPlayerState.h"

AASPlayerState::AASPlayerState()
{

}

void AASPlayerState::SetRespawnTimer_Implementation(float repsawnTime)
{
	if (Role < ROLE_Authority)
		GetWorldTimerManager().SetTimer(respawnTimer, repsawnTime, false);
}