#pragma once

#include "GameFramework/HUD.h"
#include "Quest.h"
#include "ShooterDamage.h"
#include "PlayerHUD.generated.h"

class UUserWidget;
class AAbilityShooterCharacter;
class AASPlayerState;
class AShooterItem;

UCLASS()
class APlayerHUD : public AHUD
{
	GENERATED_BODY()

protected:

	/* UMG blueprint to play for the player's HUD */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
	TSubclassOf<UUserWidget> hudMovieClass;

	/* actual instance of the current UMG movie */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HUD)
	UUserWidget* hudMovie;

	/* timer to delay when the hud is created */
	FTimerHandle hudCreateTimer;

	/* override to start up the UMG blueprint */
	virtual void BeginPlay() override;

	/* actually create the HUD */
	void CreateHUDMovie();

public:
	APlayerHUD();

	/* called whenever the current effects list of the owner is updated */
	UFUNCTION(BlueprintImplementableEvent, Category=Effects)
	void OnEffectsListUpdate();

	/* called whenever the current set of quests has updated */
	UFUNCTION(BlueprintImplementableEvent, Category = Quests)
	void OnQuestsUpdated();

	/* called whenever the a quest is completed and it is removed from the quest list and rewards are given */
	UFUNCTION(BlueprintImplementableEvent, Category = Quests)
	void OnQuestCompleted(UQuest* quest);

	/* called whenever the player looks at a game character target */
	UFUNCTION(BlueprintImplementableEvent, Category = TargetUI)
	void OnCharacterTargeted(AAbilityShooterCharacter* target);

	/* called whenever the player stops looking at a game character target */
	UFUNCTION(BlueprintImplementableEvent, Category = TargetUI)
	void OnCharacterUnTargeted();

	/* called whenever the equipment wheel should be shown for a period of time */
	UFUNCTION(BlueprintImplementableEvent, Category = Equipment)
	void OnShowEquipmentWheel();

	/* called whenever the equipment wheel needs to hide before the timeout period is finished */
	UFUNCTION(BlueprintImplementableEvent, Category = Equipment)
	void OnHideEquipmentWheel();

	/* called whenever we should play UI effects for when this player damages another character */
	UFUNCTION(BlueprintImplementableEvent, Category = Damage)
	void OnCharacterDealtDamage(FShooterDamage damage);

	/* called whenever we should play UI effects for when this player takes damatg */
	UFUNCTION(BlueprintImplementableEvent, Category = Damage)
	void OnCharacterDamaged(FShooterDamage damage);

	/* called whenever we should notify the player of a kill in the kill feed */
	UFUNCTION(BlueprintImplementableEvent, Category = Kill)
	void OnAddKillToKillfeed(AASPlayerState* killingPlayer, AShooterItem* killingItem, AASPlayerState* killedPlayer);

	/* on owning shooter died */
	UFUNCTION(BlueprintImplementableEvent, Category = Death)
	void OnCharacterDied(FShooterDamage damage);

	/* whenever the owning shooter is respawned */
	UFUNCTION(BlueprintImplementableEvent, Category = Death)
	void OnCharacterRespawned();

	/* implement displaying a message to the killfeed */
	UFUNCTION(BlueprintImplementableEvent, Category = Log)
	void ReceiveKillfeedMessage(const FString& message);
};