#include "AbilityShooter.h"
#include "AbilityShooterMovementComponent.h"
#include "AbilityShooterCharacter.h"

UAbilityShooterMovementComponent::UAbilityShooterMovementComponent()
{

}

void UAbilityShooterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCharacterDashing)
	{
		AAbilityShooterCharacter* pc = Cast<AAbilityShooterCharacter>(CharacterOwner);
		if (!IsValid(pc))
			return;

		if ((GetCharacterOwner()->GetActorLocation() - currentDashLocation).IsNearlyZero(15.f) || pc->GetCurrentAilment().type == EAilment::AL_Stun || pc->GetCurrentAilment().type == EAilment::AL_Knockup) //end the dash if we're at the target location or we got hard cc'd
			EndDash();
		else
			GetCharacterOwner()->SetActorLocation(FMath::VInterpConstantTo(GetCharacterOwner()->GetActorLocation(), currentDashLocation, DeltaTime, 7100.f*flySpeedScale));
	}
}

void UAbilityShooterMovementComponent::DashLaunch(FVector const& endLocation, float spdScale)
{
	bCharacterWantsDash = true;
	PendingLaunchVelocity = endLocation;
	flySpeedScale = spdScale;
}

void UAbilityShooterMovementComponent::EndDash()
{
	if (!bCharacterDashing)
		return;

	bCharacterDashing = false;
	currentDashLocation = FVector::ZeroVector;
	StopMovementImmediately();
	SetMovementMode(MOVE_Walking);

	AAbilityShooterCharacter* pc = Cast<AAbilityShooterCharacter>(CharacterOwner);
	if (IsValid(pc))
		pc->OnShooterDashEnded.Broadcast();
}

bool UAbilityShooterMovementComponent::HandlePendingLaunch()
{
	if (!PendingLaunchVelocity.IsZero() && HasValidData())
	{
		Velocity = PendingLaunchVelocity;
		currentDashLocation = PendingLaunchVelocity;
		PendingLaunchVelocity = FVector::ZeroVector;
		if (bCharacterWantsDash)
		{
			StopMovementImmediately();
			SetMovementMode(MOVE_Flying);
			bCharacterWantsDash = false;
			bCharacterDashing = true;
		}
		else
			SetMovementMode(MOVE_Falling);

		return true;
	}

	return false;
}