#pragma once

#include "AbilityShooterCharacter.h"
#include "AbilityShooterBot.generated.h"

UCLASS()
class AAbilityShooterBot : public AAbilityShooterCharacter
{
	GENERATED_BODY()

public:
	AAbilityShooterBot(const FObjectInitializer& objectInitializer);

	UPROPERTY(EditAnywhere, Category = Behavior)
	class UBehaviorTree* BotBehavior;

	virtual void FaceRotation(FRotator NewRotation, float DeltaTime = 0.f) override;
};