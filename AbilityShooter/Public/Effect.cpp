#include "AbilityShooter.h"
#include "Effect.h"
#include "AbilityShooterCharacter.h"

UEffect::UEffect()
{
	uiName = NSLOCTEXT("AbilityShooter", "effectUninit", "Uninitialized");
	description = NSLOCTEXT("AbilityShooter", "effectUninit", "Uninitialized");
	duration = -1.f;
	bEffectInitialized = false;
	bPersistThruDeath = false;
	key = "";
}

FText UEffect::GetUIName() const
{
	return uiName;
}

FText UEffect::GetDescription() const
{
	return description;
}

float UEffect::GetDuration() const
{
	return duration;
}

void UEffect::GetExpirationTimer(FTimerHandle& inHandle) const
{
	inHandle = expirationTimer;
}

bool UEffect::IsInitialized() const
{
	return bEffectInitialized;
}

FString UEffect::GetKey() const
{
	return key;
}

void UEffect::Initialize(const FEffectInitInfo& initInfo, AAbilityShooterCharacter* characterOwner)
{
	uiName = initInfo.uiName;
	description = initInfo.description;
	duration = initInfo.duration;
	appliedCharacter = characterOwner;
	statAlters = initInfo.statAlters;
	bPersistThruDeath = initInfo.bDoesPersistThruDeath;
	expirationTimer = initInfo.persistentTimer;
	
	bEffectInitialized = true;
}

void UEffect::SetExpirationTimer()
{
	if (IsValid(appliedCharacter))
		appliedCharacter->GetWorldTimerManager().SetTimer(expirationTimer, this, &UEffect::OnEffectExpired, duration);
}

void UEffect::OnEffectExpired()
{
	if (IsValid(appliedCharacter))
		appliedCharacter->EndEffect(this);
}