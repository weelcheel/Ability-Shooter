#pragma once

#include "Quest.h"
#include "GameFramework/PlayerInput.h"
#include "AbilityShooterCharacter.h"
#include "AbilityShooterPlayerController.generated.h"

struct FEffectInitInfo;
class AAbility;
class UStatsManager;

UCLASS()
class AAbilityShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/* quests this player currently has and that are replicated */
	UPROPERTY(BlueprintReadOnly, Category=Quests, replicated)
	TArray<UQuest*> quests;

	/* stat manager for this player to use across characters */
	UPROPERTY()
	UStatsManager* statsManager;

	/* handles restarting the player on a respawn */
	void HandleRespawnTimer();

	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

public:

	/* gets the currently controlled pawn casted to an ability character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Interp, Category = Character)
	AAbilityShooterCharacter* currentCharacter;

	/* caches the persisting effects the player has to apply to the new character on spawn */
	TArray<FEffectInitInfo> persistentEffects;

	/* caches the abilites the player has to give to the new character on spawn */
	TArray<AAbility*> persistentAbilities;

	AAbilityShooterPlayerController();

	/* server function that starts the respawn timer for this player */
	void SetRespawnTimer(float respawnTime);

	/* server function to give this player a quest */
	UFUNCTION(BlueprintCallable, Category = Quest)
	void GiveQuest(UQuest* newQuest);

	/* server function to complete a quest */
	UFUNCTION(BlueprintCallable, Category = Quest)
	void CompleteQuest(UQuest* quest);

	/* server function to give the player an objective for a quest */
	UFUNCTION(BlueprintCallable, Category = Quest)
	void GiveQuestNewObjective(UPARAM(ref) const FString& questKey, UPARAM(ref) FQuestObjective& newObjective);

	/* server function that gives the player a successful checkpoint for an objective in a quest */
	UFUNCTION(BlueprintCallable, Category = Quest)
	void GiveQuestCurrentObjectiveCheckpoint(UPARAM(ref) const FString& questKey);

	/* client function to update their HUDs quest section */
	UFUNCTION(reliable, client)
	void UpdateClientQuests();

	/* function called from quest when it is completed */
	UFUNCTION(reliable, client)
	void OnClientQuestCompleted(const FString& questKey);

	/* gets the key mappings for an input action name */
	UFUNCTION(BlueprintCallable, Category = Bindings)
	void GetKeysForAction(FName actionName, TArray<FInputActionKeyMapping>& bindings);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Pawn", meta = (Keywords = "set controller"))
	virtual void Possess(APawn* InPawn) override;
};