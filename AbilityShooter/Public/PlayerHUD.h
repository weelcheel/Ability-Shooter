#pragma once

#include "GameFramework/HUD.h"
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
};