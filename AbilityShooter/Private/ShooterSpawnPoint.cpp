#include "AbilityShooter.h"
#include "ShooterSpawnPoint.h"
#include "AbilityShooterCharacter.h"
#include "ASPlayerState.h"

AShooterSpawnPoint::AShooterSpawnPoint()
{
	scoreRange = 1000.f;
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
				if (otherState->GetTeamIndex() == player->GetTeamIndex())
					score += (1 - ((distance - GetCapsuleComponent()->GetScaledCapsuleRadius()) / (scoreRange - GetCapsuleComponent()->GetScaledCapsuleRadius())));
				else
					score -= (1 - ((distance - GetCapsuleComponent()->GetScaledCapsuleRadius()) / (scoreRange - GetCapsuleComponent()->GetScaledCapsuleRadius())));

				scoreCount++;
			}
		}
	}

	return scoreCount > 0 ? score / scoreCount : 0.f;
}