#include "AbilityShooter.h"
#include "Ability.h"
#include "AbilityShooterCharacter.h"
#include "UnrealNetwork.h"

AAbility::AAbility()
{
	//initialize the cooldown values
	currentState = EAbilityState::NoOwner;
	bAutoPerform = true;
	bUltimateAbility = false;

	bWantsToPerform = false;
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
}

bool AAbility::CanPerform() const
{
	bool bStateOK = currentState != EAbilityState::Disabled && currentState != EAbilityState::NoOwner && currentState != EAbilityState::OnCooldown;
	return bStateOK;
}

void AAbility::StartPerform()
{
	if (Role < ROLE_Authority)
		ServerStartPerform();

	if (!bWantsToPerform)
	{
		bWantsToPerform = true;
		DetermineState();
	}
}

void AAbility::StopPerform()
{
	if (Role < ROLE_Authority)
		ServerStopPerform();

	if (bWantsToPerform)
	{
		bWantsToPerform = false;
		DetermineState();
	}
}

bool AAbility::ServerStartPerform_Validate()
{
	return true;
}

void AAbility::ServerStartPerform_Implementation()
{
	StartPerform();
}

bool AAbility::ServerStopPerform_Validate()
{
	return true;
}

void AAbility::ServerStopPerform_Implementation()
{
	StopPerform();
}

void AAbility::DetermineState()
{
	EAbilityState newState = EAbilityState::Idle;

	if (currentState == EAbilityState::OnCooldown)
		newState = EAbilityState::OnCooldown;
	else if (currentState == EAbilityState::Disabled)
		newState = EAbilityState::Disabled;
	else if (IsValid(characterOwner))
	{
		if (bWantsToPerform && CanPerform() && characterOwner->CanPerformAbilities())
			newState = EAbilityState::Performing;
	}
	else
		newState = EAbilityState::NoOwner;

	SetState(newState);
}

void AAbility::SetState(EAbilityState newState)
{
	const EAbilityState prevState = currentState;

	if (prevState == EAbilityState::Performing && newState != EAbilityState::Performing)
		OnStopPerform();

	currentState = newState;

	if (prevState != EAbilityState::Performing && currentState == EAbilityState::Performing)
		OnStartPerform();
}

void AAbility::SetupAbility(AAbilityShooterCharacter* newOwner)
{
	if (Role < ROLE_Authority)
		return;

	SetOwner(newOwner);
	FAttachmentTransformRules rules(EAttachmentRule::SnapToTarget, false);
	AttachToActor(newOwner, rules);

	characterOwner = newOwner;
	currentState = EAbilityState::Idle;
}

void AAbility::AddVeteranLevel()
{
	if (Role < ROLE_Authority)
		return;

	if (veteranLevel + 1 <= maxVeteranLevel)
		veteranLevel++;
}

float AAbility::GetVeteranLevelScaledValue(TArray<float>& values) const
{
	if (veteranLevel >= 0 && values.Num() > 0 && veteranLevel < values.Num())
		return values[veteranLevel];
	else
		return 0.f;
}

float AAbility::GetCooldownProgressPercent() const
{
	if (currentState != EAbilityState::OnCooldown)
		return 0.f;

	float timeElapsed = GetWorldTimerManager().GetTimerElapsed(cooldownTimer);
	float totalTime = timeElapsed + GetWorldTimerManager().GetTimerRemaining(cooldownTimer);

	return timeElapsed / totalTime;
}

float AAbility::GetCooldownRemaining() const
{
	if (currentState != EAbilityState::OnCooldown)
		return 0.f;

	return GetWorldTimerManager().GetTimerRemaining(cooldownTimer);
}

void AAbility::StartCooldown_Implementation(float manualCooldown /* = -1.f */, EAbilityState cdfState /* = EAbilityState::Idle */)
{
	currentState = EAbilityState::OnCooldown;
	afterCooldownState = cdfState;

	float duration;
	if (manualCooldown == 0.f)
	{
		currentState = afterCooldownState;
		return;
	}
	else
		duration = manualCooldown > 0.f ? manualCooldown : GetVeteranLevelScaledValue(baseCooldownTimes);

	//@TODO: let cooldown reduction modify duration
	GetWorldTimerManager().SetTimer(cooldownTimer, this, &AAbility::CooldownFinished, duration);
}

void AAbility::CooldownFinished()
{
	currentState = afterCooldownState;
}

void AAbility::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbility, characterOwner);
	DOREPLIFETIME(AAbility, veteranLevel);
}