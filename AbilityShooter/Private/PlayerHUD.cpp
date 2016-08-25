#include "AbilityShooter.h"
#include "PlayerHUD.h"
#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

APlayerHUD::APlayerHUD()
{

}

void APlayerHUD::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(hudCreateTimer, this, &APlayerHUD::CreateHUDMovie, 2.f);
}

void APlayerHUD::CreateHUDMovie()
{
	if (IsValid(hudMovieClass))
		hudMovie = CreateWidget<UUserWidget>(PlayerOwner, hudMovieClass);

	hudMovie->AddToViewport();
}