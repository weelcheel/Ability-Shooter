#pragma once

#include "GameFramework/PlayerState.h"
#include "ASPlayerState.generated.h"

UCLASS()
class AASPlayerState : public APlayerState
{
	friend class AAbilityShooterPlayerController;

	GENERATED_BODY()

protected:

	/* respawn timer handle that will be set when a net multicast function is called from the server on this player state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Respawn)
	FTimerHandle respawnTimer;

public:
	AASPlayerState();

	/* net multicast function to set the respawn timer on all clients but actually performs the respawn on the server */
	UFUNCTION(NetMulticast, reliable)
	void SetRespawnTimer(float repsawnTime);
};