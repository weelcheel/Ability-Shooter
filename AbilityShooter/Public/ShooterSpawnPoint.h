#pragma once

#include "GameFramework/PlayerStart.h"
#include "ShooterSpawnPoint.generated.h"

class AASPlayerState;

UCLASS(Blueprintable)
class AShooterSpawnPoint : public APlayerStart
{
	GENERATED_BODY()

protected:

	/* range that this spawn point considers Shooters for scoring */
	UPROPERTY(EditAnywhere, Category = Range)
	float scoreRange;

public:
	AShooterSpawnPoint();

	/* team this is a spawn point for */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Team)
	int32 teamIndex;

	/* gets a score between -1 and 1 rating this spawn point for a given player state */
	float GetSpawnScoreForPlayer(AASPlayerState* player) const;
};