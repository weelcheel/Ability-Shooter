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

	/* team index this player is on if it's needed for a team game */
	UPROPERTY(replicated)
	int32 team;

public:
	AASPlayerState();
	
	/* amount of cash this player has */
	UPROPERTY(replicated, BlueprintReadWrite, Category = Cash)
	int32 cash;

	/* net multicast function to set the respawn timer on all clients but actually performs the respawn on the server */
	UFUNCTION(NetMulticast, reliable)
	void SetRespawnTimer(float repsawnTime);

	/* gets the team index for this player */
	UFUNCTION(BlueprintCallable, Category = Team)
	int32 GetTeamIndex() const;

	/* sets the team index */
	void SetTeamIndex(int32 newTeam);
};