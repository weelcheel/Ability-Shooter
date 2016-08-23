#pragma once

#include "AbilityShooterPlayerController.generated.h"

UCLASS()
class AAbilityShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/* handles restarting the player on a respawn */
	void HandleRespawnTimer();

public:
	AAbilityShooterPlayerController();

	/* server function that starts the respawn timer for this player */
	void SetRespawnTimer(float respawnTime);
};