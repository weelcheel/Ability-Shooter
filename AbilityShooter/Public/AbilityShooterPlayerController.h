#pragma once

#include "AbilityShooterPlayerController.generated.h"

struct FEffectInitInfo;

UCLASS()
class AAbilityShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/* handles restarting the player on a respawn */
	void HandleRespawnTimer();

public:

	/* caches the persisting effects the player has to apply to the new character on spawn */
	TArray<FEffectInitInfo> persistentEffects;

	AAbilityShooterPlayerController();

	/* server function that starts the respawn timer for this player */
	void SetRespawnTimer(float respawnTime);
};