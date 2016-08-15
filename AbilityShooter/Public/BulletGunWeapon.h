#pragma once

#include "Weapon.h"
#include "BulletGunWeapon.generated.h"

USTRUCT()
struct FBulletWeaponData
{
	GENERATED_USTRUCT_BODY()

	/* infinite ammo for reloading */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool bInfiniteAmmo;

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool bInfiniteClip;

	/** max amount of total ammo */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 maxAmmo;

	/** clip size */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 ammoPerClip;

	/** initial clips */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 initialClips;

	/** failsafe reload duration if weapon doesn't have any animation for it */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float noAnimReloadDuration;

	FBulletWeaponData()
	{
		bInfiniteAmmo = false;
		bInfiniteClip = false;
		maxAmmo = 320;
		ammoPerClip = 32;
		initialClips = 4;
		noAnimReloadDuration = 1.f;
	}
};

UCLASS(ABSTRACT, Blueprintable)
class ABulletGunWeapon : public AWeapon
{
	GENERATED_BODY()

protected:

	/** is reload animation playing? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
	uint32 bPendingReload : 1;

	/** name of bone/socket for muzzle in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName muzzleAttachPoint;

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* muzzleFX;

	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
	UParticleSystemComponent* muzzlePSC;

	/** out of ammo sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* outOfAmmoSound;

	/** reload sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* reloadSound;

	/** reload animations */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* reloadAnim;

	/** is muzzle FX looped? */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	uint32 bLoopedMuzzleFX : 1;

	/** current total ammo */
	UPROPERTY(Transient, Replicated)
	int32 currentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(Transient, Replicated)
	int32 currentAmmoInClip;

	/** weapon data */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FBulletWeaponData weaponConfig;

	FTimerHandle reloadTimer;
	FTimerHandle stopReloadTimer;

	/** perform initial setup */
	virtual void PostInitializeComponents() override;

	/** [server] add ammo */
	void GiveAmmo(int addAmount);

	/** consume a bullet */
	void UseAmmo();

	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [all] start weapon reload */
	virtual void StartReload(bool bFromReplication = false);

	/** [local + server] interrupt weapon reload */
	virtual void StopReload();

	/** [server] performs actual reload */
	virtual void ReloadWeapon();

	/** trigger reload from server */
	UFUNCTION(reliable, client)
	void ClientStartReload();

	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartReload();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopReload();

	//////////////////////////////////////////////////////////////////////////
	// Control

	/** check if weapon can be reloaded */
	bool CanReload() const;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/* add more state logic for reloading */
	virtual void DetermineEquipmentState() override;

	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_Reload();

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** get the muzzle location of the weapon */
	FVector GetMuzzleLocation() const;

	/** get direction of weapon's muzzle */
	FVector GetMuzzleDirection() const;

public:

	ABulletGunWeapon();
};