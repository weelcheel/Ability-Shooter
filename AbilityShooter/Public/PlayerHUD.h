#pragma once

#include "GameFramework/HUD.h"
#include "Quest.h"
#include "PlayerHUD.generated.h"

class UUserWidget;

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
};