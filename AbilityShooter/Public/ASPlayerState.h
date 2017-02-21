#pragma once

#include "GameFramework/PlayerState.h"
#include "ASPlayerState.generated.h"

class AShooterItem;
class AAbilityShooterCharacter;

UCLASS()
class AASPlayerState : public APlayerState
{
	friend class AAbilityShooterPlayerController;
	friend class AAbilityShooterMainMenu;

	GENERATED_BODY()

protected:

	/* respawn timer handle that will be set when a net multicast function is called from the server on this player state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Respawn)
	FTimerHandle respawnTimer;

	/* team index this player is on if it's needed for a team game */
	UPROPERTY(replicated)
	int32 team;

	/* string version of the json object that is the player's profile */
	UPROPERTY(replicated)
	FString profileJson;

public:
	AASPlayerState();
	
	/* amount of cash this player has */
	UPROPERTY(replicated, BlueprintReadWrite, Category = Cash)
	int32 cash;

	/* current view rotation for this player so that their aim is successfully replicated */
	UPROPERTY(BlueprintReadOnly, Category = ViewRotation)
	FRotator viewRotation;

	/* current Shooter this player is currently controlling */
	UPROPERTY(BlueprintReadOnly, replicated, Category = Shooter)
	AAbilityShooterCharacter* currentShooter;

	/* net multicast function to set the respawn timer on all clients but actually performs the respawn on the server */
	UFUNCTION(NetMulticast, reliable)
	void SetRespawnTimer(float repsawnTime);

	/* net multicast function to notify the local hud to add a kill to the killfeed */
	UFUNCTION(NetMulticast, reliable)
	void BroadcastKillToHUD(AASPlayerState* killingPlayer, AShooterItem* killingItem, AASPlayerState* killedPlayer);

	/* gets the team index for this player */
	UFUNCTION(BlueprintCallable, Category = Team)
	int32 GetTeamIndex() const;

	/* sets the team index */
	void SetTeamIndex(int32 newTeam);

	/* gets a string version of the unique id */
	UFUNCTION(BlueprintCallable, Category = UID)
	FString GetUniqueIDString();
};