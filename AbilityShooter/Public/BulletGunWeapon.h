#pragma once

#include "Weapon.h"
#include "BulletGunWeapon.generated.h"

USTRUCT()
struct FInstantHitInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector origin;

	UPROPERTY()
	float reticleSpread;

	UPROPERTY()
	int32 randomSeed;
};

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

	/** base weapon spread (degrees) */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float weaponSpread;

	/** targeting spread modifier */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float targetingSpreadMod;

	/** continuous firing: spread increment */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float firingSpreadIncrement;

	/** continuous firing: max increment */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float firingSpreadMax;

	/** weapon range */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float weaponRange;

	/** damage amount */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	int32 hitDamage;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> damageType;

	/** hit verification: scale for bounding box of hit actor */
	UPROPERTY(EditDefaultsOnly, Category = HitVerification)
	float clientSideHitLeeway;

	/** hit verification: threshold for dot product between view direction and hit direction */
	UPROPERTY(EditDefaultsOnly, Category = HitVerification)
	float allowedViewDotHitDir;

	/** scale to multiply or divide the camera boom length by for aiming */
	UPROPERTY(EditDefaultsOnly, Category = HitVerification)
	float aimingScale;

	FBulletWeaponData()
	{
		bInfiniteAmmo = false;
		bInfiniteClip = false;
		maxAmmo = 320;
		ammoPerClip = 32;
		initialClips = 4;
		noAnimReloadDuration = 1.f;

		weaponSpread = 5.0f;
		targetingSpreadMod = 0.25f;
		firingSpreadIncrement = 1.0f;
		firingSpreadMax = 10.0f;
		weaponRange = 10000.0f;
		hitDamage = 10;
		damageType = UDamageType::StaticClass();
		clientSideHitLeeway = 200.0f;
		allowedViewDotHitDir = 0.8f;
		aimingScale = 0.5f;
	}
};

UCLASS(ABSTRACT, Blueprintable)
class ABulletGunWeapon : public AWeapon
{
	GENERATED_BODY()

protected:

	/** instant hit notify for replication */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_HitNotify)
	FInstantHitInfo hitNotify;

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

	/** smoke trail */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* trailFX;

	/** param name for beam target in smoke trail */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName trailTargetParam;

	/** current spread from continuous firing */
	float currentFiringSpread;

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

	/** check if weapon can be fired */
	virtual bool CanUse() const override;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/* add more state logic for reloading */
	virtual void DetermineEquipmentState() override;

	/** server notified of hit from client to verify */
	UFUNCTION(reliable, server, WithValidation)
	void ServerNotifyHit(const FHitResult Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	/** server notified of miss to show trail FX */
	UFUNCTION(unreliable, server, WithValidation)
	void ServerNotifyMiss(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	/** process the instant hit and notify the server if necessary */
	void ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread);

	/** continue processing the instant hit, as if it has been confirmed by the server */
	void ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread);

	/** check if weapon should deal damage to actor */
	bool ShouldDealDamage(AActor* TestActor) const;

	/** handle damage */
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir);

	/** [local] weapon specific fire implementation */
	virtual void UseEquipment() override;

	/* [local] instant hit weapon fire logic */
	void FireWeapon();

	/** [local + server] update spread on firing */
	virtual void OnBurstFinished() override;

	/** [local + server] handle weapon firing */
	virtual void HandleUsing();

	virtual void OnAltStarted() override;
	virtual void OnAltFinished() override;

	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_Reload();

	UFUNCTION()
	void OnRep_HitNotify();

	/** called in network play to do the cosmetic fx  */
	void SimulateInstantHit(const FVector& Origin, int32 RandomSeed, float ReticleSpread);

	/** spawn effects for impact */
	void SpawnImpactEffects(const FHitResult& Impact);

	/** spawn trail effect */
	void SpawnTrailEffect(const FVector& EndPoint);

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** get the muzzle location of the weapon */
	FVector GetMuzzleLocation() const;

	/** get direction of weapon's muzzle */
	FVector GetMuzzleDirection() const;

public:

	ABulletGunWeapon();
};