// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/GameMode.h"
#include "AbilityShooterGameMode.generated.h"

UCLASS(minimalapi)
class AAbilityShooterGameMode : public AGameMode
{
	GENERATED_BODY()

protected:

	/* base time value for player respawns */
	UPROPERTY(EditDefaultsOnly, Category = Respawn)
	float basePlayerRespawnTime;

	/* gets the respawn timer for the player */
	float GetRespawnTime(class AAbilityShooterPlayerController* player) const;

public:
	AAbilityShooterGameMode();

	/* called whenever a Shooter is killed in game */
	void ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType);
};



