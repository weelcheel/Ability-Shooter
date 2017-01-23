// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameFramework/GameMode.h"
#include "StoreItem.h"
#include "AbilityShooterBot.h"
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

	/* base amount of cash assigned per kill */
	UPROPERTY(EditDefaultsOnly, Category = Goals)
	int32 baseCashPerKill;

	/* amount of cash extra first kill is worth */
	UPROPERTY(EditDefaultsOnly, Category = Goals)
	int32 firstBloodBonus;

	/* class of shooter character to use for AI bots */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	TSubclassOf<AAbilityShooterBot> aiShooterBotClass;

	/* check to see if there is enough room for the player */
	virtual void PreLogin(const FString& Options, const FString& Address, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage);

	/* give the player the list of store items */
	virtual void PostLogin(APlayerController* newPlayer) override;

	/* finds the best shooter spawn point for the player */
	virtual AActor* FindBestShooterSpawn(class AController* player);

public:
	AAbilityShooterGameMode();

	/* string path that allows servers to easily start this game mode from command line */
	UPROPERTY(EditDefaultsOnly, Category = Path)
	FString gameModeCmdPath;

	/* called whenever a Shooter is killed in game */
	virtual void ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType, class AShooterItem* killingItem);

	/* modify the amount of cash that this kill is worth */
	virtual void ModifyKillReward(int32& cashReward);

	/* can one player state deal damage to another */
	virtual bool CanDealDamage(AASPlayerState* damageInstigator, AASPlayerState* damagedPlayer) const;

	/* handle new respawn algorithm */
	virtual void RestartPlayer(class AController* NewPlayer) override;

	/* gets the respawn timer for the player */
	virtual float GetRespawnTime(class AController* player) const;

	/* start the match after pre game lobby */
	virtual void StartMatch() override;

	/* end the match and declare a winner */
	virtual void EndMatch() override;

	/* decide the winner of this match */
	virtual int32 DecideWinner();
};



