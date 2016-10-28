#pragma once

#include "AbilityShooterGameMode.h"
#include "TeamGameMode.generated.h"

UCLASS(Blueprintable)
class ATeamGameMode : public AAbilityShooterGameMode
{
	GENERATED_BODY()

protected:

	/* number of teams this game mode has */
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	int32 teamCount;

	/* max number of players per team */
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	int32 maxNumTeamPlayers;

	/* number of players on each team */
	UPROPERTY(BlueprintReadOnly, Category = Team)
	TArray<int32> numTeamPlayers;

	/* assign the player a team */
	virtual void PostLogin(APlayerController* newPlayer) override;

	/* finds the best spawn point including teams */
	virtual AActor* FindBestShooterSpawn(class AController* player) override;

public:
	ATeamGameMode();

	/* can one player state deal damage to another */
	virtual bool CanDealDamage(AASPlayerState* damageInstigator, AASPlayerState* damagedPlayer) const;
};