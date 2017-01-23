#include "AbilityShooter.h"
#include "ASPlayerState.h"
#include "UnrealNetwork.h"
#include "AbilityShooterPlayerController.h"
#include "PlayerHUD.h"
#include "ShooterItem.h"

AASPlayerState::AASPlayerState()
{

}

void AASPlayerState::SetRespawnTimer_Implementation(float repsawnTime)
{
	if (Role < ROLE_Authority)
		GetWorldTimerManager().SetTimer(respawnTimer, repsawnTime, false);
}

int32 AASPlayerState::GetTeamIndex() const
{
	return team;
}

void AASPlayerState::SetTeamIndex(int32 newTeam)
{
	team = newTeam;
}

FString AASPlayerState::GetUniqueIDString()
{
	return UniqueId->ToString();
}

void AASPlayerState::BroadcastKillToHUD_Implementation(AASPlayerState* killingPlayer, AShooterItem* killingItem, AASPlayerState* killedPlayer)
{
	APlayerController* pc = GetWorld()->GetFirstPlayerController();
	if (IsValid(pc))
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (IsValid(hud))
			hud->OnAddKillToKillfeed(killingPlayer, killingItem, killedPlayer);
	}
}

void AASPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// everyone
	DOREPLIFETIME(AASPlayerState, team);
	DOREPLIFETIME(AASPlayerState, cash);
	DOREPLIFETIME(AASPlayerState, profileJson);
	//DOREPLIFETIME(AASPlayerState, viewRotation);
}