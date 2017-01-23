#pragma once

#include "GameFramework/GameState.h"
#include "AbilityShooterGameState.generated.h"

UCLASS()
class AAbilityShooterGameState : public AGameState
{
	GENERATED_BODY()

public:
	AAbilityShooterGameState();

	UPROPERTY(BlueprintReadOnly, replicated, Category = Goals)
	int32 scoreGoal;

	UPROPERTY(BlueprintReadOnly, replicated, Category = Goals)
	bool bIsTeamGame;

	UPROPERTY(BlueprintReadWrite, replicated, Category = AI)
	int32 numOfBots;
};