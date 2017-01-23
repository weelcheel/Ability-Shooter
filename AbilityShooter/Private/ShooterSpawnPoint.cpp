#include "AbilityShooter.h"
#include "ShooterSpawnPoint.h"
#include "AbilityShooterCharacter.h"
#include "ASPlayerState.h"

AShooterSpawnPoint::AShooterSpawnPoint()
{
	scoreRange = 10000.f;
	teamIndex = -1;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

float AShooterSpawnPoint::GetSpawnScoreForPlayer(AASPlayerState* player) const
{
	if (!IsValid(player))
		return 0.f;

	float score = 0.f;
	int32 scoreCount = 0;

	for (TActorIterator<AAbilityShooterCharacter> itr(GetWorld()); itr; ++itr)
	{
		AAbilityShooterCharacter* pc = (*itr);
		if (IsValid(pc))
		{
			//continue if this Shooter's not in range
			float distance = (pc->GetActorLocation() - GetActorLocation()).Size();
			if (distance > scoreRange)
				continue;

			AASPlayerState* otherState = Cast<AASPlayerState>(pc->PlayerState);
			if (IsValid(otherState))
			{
				int32 thisTeam = player->GetTeamIndex();
				int32 otherTeam = otherState->GetTeamIndex();

				if (thisTeam == otherTeam)
					score += (1 - ((distance - GetCapsuleComponent()->GetScaledCapsuleRadius()) / (scoreRange - GetCapsuleComponent()->GetScaledCapsuleRadius())));
				else
					score -= (1 - ((distance - GetCapsuleComponent()->GetScaledCapsuleRadius()) / (scoreRange - GetCapsuleComponent()->GetScaledCapsuleRadius())));

				scoreCount++;
			}
		}
	}


	float avgScore;
	if (scoreCount > 0)
		avgScore = score / scoreCount;
	else
		avgScore = 0.f;

	TArray<AActor*> overlappingCharacters;
	GetCapsuleComponent()->GetOverlappingActors(overlappingCharacters, AAbilityShooterCharacter::StaticClass());

	if (overlappingCharacters.Num() > 0)
		avgScore++;

	return avgScore;
}