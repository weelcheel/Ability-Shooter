#include "AbilityShooter.h"
#include "AbilityShooterPlayerController.h"
#include "ASPlayerState.h"
#include "AbilityShooterGameMode.h"

AAbilityShooterPlayerController::AAbilityShooterPlayerController()
{

}

void AAbilityShooterPlayerController::SetRespawnTimer(float respawnTime)
{
	if (Role < ROLE_Authority)
		return;

	AASPlayerState* ps = Cast<AASPlayerState>(PlayerState);
	if (IsValid(ps))
	{
		//set the server version of the respawn timer that actually restarts the player
		GetWorldTimerManager().SetTimer(ps->respawnTimer, this, &AAbilityShooterPlayerController::HandleRespawnTimer, respawnTime);

		//now set the visual timer for the clients
		ps->SetRespawnTimer(respawnTime);
	}
}

void AAbilityShooterPlayerController::HandleRespawnTimer()
{
	if (Role == ROLE_Authority)
	{
		AAbilityShooterGameMode* gm = GetWorld()->GetAuthGameMode<AAbilityShooterGameMode>();
		if (IsValid(gm))
			gm->RestartPlayer(this);
	}
}