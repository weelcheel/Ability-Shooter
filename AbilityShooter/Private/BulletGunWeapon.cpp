#include "AbilityShooter.h"
#include "BulletGunWeapon.h"
#include "UnrealNetwork.h"
#include "AbilityShooterCharacter.h"

ABulletGunWeapon::ABulletGunWeapon()
{
	bLoopedMuzzleFX = false;
	bPendingReload = false;

	currentAmmo = 0;
	currentAmmoInClip = 0;

	weaponType = EWeaponType::BulletGun;
}

void ABulletGunWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (weaponConfig.initialClips > 0)
	{
		currentAmmoInClip = weaponConfig.ammoPerClip;
		currentAmmo = weaponConfig.ammoPerClip * (weaponConfig.initialClips - 1);
	}
}

void ABulletGunWeapon::StartReload(bool bFromReplication /* = false */)
{
	if (!bFromReplication && Role < ROLE_Authority)
		ServerStartReload();

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineEquipmentState();

		float animDuration = PlayEquipmentAnimation(reloadAnim);
		if (animDuration <= 0.f)
			animDuration = weaponConfig.noAnimReloadDuration;

		GetWorldTimerManager().SetTimer(stopReloadTimer, this, &ABulletGunWeapon::StopReload, animDuration, false);
		if (Role == ROLE_Authority)
			GetWorldTimerManager().SetTimer(reloadTimer, this, &ABulletGunWeapon::ReloadWeapon, FMath::Max(0.1f, animDuration - 0.1f), false);

		if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
			PlayEquipmentSound(reloadSound);
	}
}

void ABulletGunWeapon::StopReload()
{
	if (currentState == EEquipmentState::Reloading)
	{
		bPendingReload = false;
		DetermineEquipmentState();
		StopEquipmentAnimation(reloadAnim);
	}
}

bool ABulletGunWeapon::ServerStartReload_Validate()
{
	return true;
}

void ABulletGunWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool ABulletGunWeapon::ServerStopReload_Validate()
{
	return true;
}

void ABulletGunWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void ABulletGunWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

bool ABulletGunWeapon::CanReload() const
{
	bool bCanReload = !characterOwner || characterOwner->CanReload();
	bool bHasAmmo = currentAmmoInClip < weaponConfig.ammoPerClip && (currentAmmo - currentAmmoInClip > 0 || weaponConfig.bInfiniteClip);
	bool bStateOk = currentState == EEquipmentState::Idle || currentState == EEquipmentState::Using;

	return bCanReload && bHasAmmo && bStateOk;
}

void ABulletGunWeapon::GiveAmmo(int addAmount)
{
	const int32 missingAmmo = FMath::Max(0, weaponConfig.maxAmmo - currentAmmo);
	addAmount = FMath::Min(addAmount, missingAmmo);
	currentAmmo += addAmount;

	//@TODO: tell AI to check ammo

	//reload if clip was empty
	if (currentAmmoInClip <= 0 && CanReload() && characterOwner->GetCurrentEquipment() == this)
		ClientStartReload();
}

void ABulletGunWeapon::UseAmmo()
{
	if (!weaponConfig.bInfiniteAmmo)
		currentAmmoInClip--;

	if (!weaponConfig.bInfiniteAmmo && !weaponConfig.bInfiniteClip)
		currentAmmo--;

	//@TODO: AI check ammo
}

void ABulletGunWeapon::ReloadWeapon()
{
	int32 clipDelta = FMath::Min(weaponConfig.ammoPerClip - currentAmmoInClip, currentAmmo - currentAmmoInClip);

	if (weaponConfig.bInfiniteClip)
		clipDelta = weaponConfig.ammoPerClip - currentAmmoInClip;

	if (clipDelta > 0)
		currentAmmoInClip += clipDelta;

	if (weaponConfig.bInfiniteClip)
		currentAmmo = FMath::Max(currentAmmoInClip, currentAmmo);
}

void ABulletGunWeapon::DetermineEquipmentState()
{
	EEquipmentState newState = EEquipmentState::Idle;
	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (!CanReload())
				newState = currentState;
			else
				newState = EEquipmentState::Reloading;
		}
		else if (!bPendingReload && bWantsToUse && CanUse())
			newState = EEquipmentState::Using;
	}
	else if (bPendingEquip)
		newState = EEquipmentState::Equipping;

	SetEquipmentState(newState);
}

FVector ABulletGunWeapon::GetMuzzleLocation() const
{
	USkeletalMeshComponent* useMesh = GetEquipmentMesh();
	return useMesh->GetSocketLocation(muzzleAttachPoint);
}

FVector ABulletGunWeapon::GetMuzzleDirection() const
{
	USkeletalMeshComponent* useMesh = GetEquipmentMesh();
	return useMesh->GetSocketRotation(muzzleAttachPoint).Vector();
}

void ABulletGunWeapon::OnRep_Reload()
{
	if (bPendingReload)
		StartReload(true);
	else
		StopReload();
}

void ABulletGunWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABulletGunWeapon, currentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABulletGunWeapon, currentAmmoInClip, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ABulletGunWeapon, bPendingReload, COND_SkipOwner);
}