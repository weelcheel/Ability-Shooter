#include "AbilityShooter.h"
#include "AbilityShooterPlayerController.h"
#include "ASPlayerState.h"
#include "AbilityShooterGameMode.h"
#include "AbilityShooterCharacter.h"
#include "Effect.h"

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
		{
			gm->RestartPlayer(this);
			
			AAbilityShooterCharacter* ownedCharacter = Cast<AAbilityShooterCharacter>(GetCharacter());
			if (IsValid(ownedCharacter))
			{
				for (int32 i = 0; i < persistentEffects.Num(); i++)
				{
					if (persistentEffects[i].persistentTime >= 0.f)
					{
						persistentEffects[i].duration -= GetWorld()->TimeSeconds - persistentEffects[i].persistentTime;
						if (persistentEffects[i].duration <= 0.f)
							continue;
					}
						
					ownedCharacter->ApplyEffect(nullptr, persistentEffects[i]);
				}

				persistentEffects.Empty();
			}
		}
	}
}