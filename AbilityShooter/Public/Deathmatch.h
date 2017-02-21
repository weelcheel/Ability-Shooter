#pragma once

#include "AbilityShooterGameMode.h"
#include "Deathmatch.generated.h"

UCLASS(Blueprintable)
class ADeathmatch : public AAbilityShooterGameMode
{
	GENERATED_BODY()

	/* whether or not this game has already had a first blood */
	bool bFirstBloodRewarded = false;

	/* winner of the match */
	AController* winner;

protected:

	/* what score a Shooter must reach to win the game */
	UPROPERTY(EditDefaultsOnly, Category = Goals)
	int32 scoreGoal;

	/* how much score each kill is worth */
	UPROPERTY(EditDefaultsOnly, Category = Goals)
	int32 killScore;

	/* how much score each death is worth */
	UPROPERTY(EditDefaultsOnly, Category = Goals)
	int32 deathScore;

public:
	ADeathmatch();

	/* called whenever a Shooter is killed in game and tally scores */
	virtual void ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType, class AShooterItem* killingItem) override;

	/* modify the amount of cash that this kill is worth */
	virtual void ModifyKillReward(int32& cashReward) override;

	/* set the first blood bool to false at the start of matches (just in case we reset on same map) */
	virtual void StartMatch() override;
};