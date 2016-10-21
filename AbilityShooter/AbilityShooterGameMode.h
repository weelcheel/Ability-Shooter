// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameFramework/GameMode.h"
#include "StoreItem.h"
#include "AbilityShooterGameMode.generated.h"

class AASPlayerState;

UCLASS(minimalapi)
class AAbilityShooterGameMode : public AGameMode
{
	GENERATED_BODY()

protected:

	/* base time value for player respawns */
	UPROPERTY(EditDefaultsOnly, Category = Respawn)
	float basePlayerRespawnTime;

	/* list of store items to sell in-game */
	UPROPERTY(EditDefaultsOnly, Category = IngameStore)
	TArray<FStoreItem> storeItems;

	/* gets the respawn timer for the player */
	float GetRespawnTime(class AAbilityShooterPlayerController* player) const;

	/* give the player the list of store items */
	virtual void PostLogin(APlayerController* newPlayer) override;

public:
	AAbilityShooterGameMode();

	/* called whenever a Shooter is killed in game */
	void ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType);

	/* can one player state deal damage to another */
	bool CanDealDamage(AASPlayerState* damageInstigator, AASPlayerState* damagedPlayer) const;
};



