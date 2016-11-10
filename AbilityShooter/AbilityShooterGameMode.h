// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameFramework/GameMode.h"
#include "StoreItem.h"
#include "AbilityShooterGameMode.generated.h"

class AShooterSpawnPoint;

USTRUCT()
struct FSpawnPointEntry
{
	GENERATED_USTRUCT_BODY()

	/* spawn point */
	AShooterSpawnPoint* spawn;

	/* score for the spawn */
	float score;

	bool operator< (const FSpawnPointEntry& rhs) const
	{ 
		return score < rhs.score;
	}
};

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

	/* max number of players that can be active in this server */
	UPROPERTY(EditDefaultsOnly, Category = PlayerCount)
	int32 maxNumPlayers;

	/* check to see if there is enough room for the player */
	virtual void PreLogin(const FString& Options, const FString& Address, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage);

	/* give the player the list of store items */
	virtual void PostLogin(APlayerController* newPlayer) override;

	/* finds the best shooter spawn point for the player */
	virtual AActor* FindBestShooterSpawn(class AController* player);

public:
	AAbilityShooterGameMode();

	/* called whenever a Shooter is killed in game */
	void ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType);

	/* can one player state deal damage to another */
	virtual bool CanDealDamage(AASPlayerState* damageInstigator, AASPlayerState* damagedPlayer) const;

	/* handle new respawn algorithm */
	virtual void RestartPlayer(class AController* NewPlayer) override;

	/* gets the respawn timer for the player */
	virtual float GetRespawnTime(class AController* player) const;
};



