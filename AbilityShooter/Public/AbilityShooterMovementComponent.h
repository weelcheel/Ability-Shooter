#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "AbilityShooterMovementComponent.generated.h"

UCLASS()
class UAbilityShooterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:

	/* tick function */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/* current fly speed scale for dashes */
	float flySpeedScale = 1.f;

	/* whether or not we want to dash */
	bool bCharacterWantsDash = false;

	/* whether or not we're currently dashing */
	bool bCharacterDashing = false;

	/* location we're currently dashing to */
	FVector currentDashLocation;

public:
	UAbilityShooterMovementComponent();

	/* launch a character dash */
	void DashLaunch(FVector const& endLocation, float spdScale = 1.f);

	/* end a dash in progress */
	void EndDash();

	virtual bool HandlePendingLaunch() override;
};