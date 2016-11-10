#include "AbilityShooter.h"
#include "AbilityShooterBot.h"
#include "AbilityAIController.h"

AAbilityShooterBot::AAbilityShooterBot()
{
	AIControllerClass = AAbilityAIController::StaticClass();
}

void AAbilityShooterBot::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	FRotator CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);

	Super::FaceRotation(CurrentRotation, DeltaTime);
}