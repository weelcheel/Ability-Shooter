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
	currentFiringSpread = 0.f;

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

bool ABulletGunWeapon::CanUse() const
{
	bool bCanFire = IsValid(characterOwner) && characterOwner->CanUseEquipment();
	bool bStateOk = currentState == EEquipmentState::Idle || currentState == EEquipmentState::Using;
	return bCanFire && bStateOk && !bPendingReload;
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

void ABulletGunWeapon::UseEquipment()
{
	FireWeapon();
}

void ABulletGunWeapon::FireWeapon()
{
	const int32 randomSeed = FMath::Rand();
	FRandomStream weapRandom(randomSeed);
	const float currentSpread = currentFiringSpread;
	const float coneHalfAngle = FMath::DegreesToRadians(currentSpread * 0.5f);

	const FVector aimDir = GetAdjustedAim();
	const FVector startTrace = GetCameraDamageStartLocation(aimDir);
	const FVector shootDir = weapRandom.VRandCone(aimDir, coneHalfAngle, coneHalfAngle);
	const FVector endTrace = startTrace + shootDir * weaponConfig.weaponRange;

	const FHitResult impact = EquipmentTrace(startTrace, endTrace);
	ProcessInstantHit(impact, startTrace, shootDir, randomSeed, currentSpread);

	currentFiringSpread = FMath::Min(weaponConfig.firingSpreadMax, currentFiringSpread + weaponConfig.firingSpreadIncrement);
}

bool ABulletGunWeapon::ServerNotifyHit_Validate(const FHitResult Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	return true;
}

void ABulletGunWeapon::ServerNotifyHit_Implementation(const FHitResult Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	const float weaponAngleDot = FMath::Abs(FMath::Sin(ReticleSpread * PI / 180.f));

	//calculate dot product between view and the shot, if available
	if (IsValid(Instigator) && (IsValid(Impact.GetActor()) || Impact.bBlockingHit))
	{
		const FVector origin = GetMuzzleLocation();
		const FVector viewDir = (Impact.Location - origin).GetSafeNormal();

		//is the angle between the hit and view within the allowed limits? (limit + weapon max angle)
		const float viewDotHitDir = FVector::DotProduct(Instigator->GetViewRotation().Vector(), viewDir);
		if (viewDotHitDir > weaponConfig.allowedViewDotHitDir - weaponAngleDot)
		{
			if (currentState != EEquipmentState::Idle)
			{
				if (!IsValid(Impact.GetActor()))
				{
					if (Impact.bBlockingHit)
						ProcessInstantHit_Confirmed(Impact, origin, ShootDir, RandomSeed, ReticleSpread);
				}
			}
			else if (Impact.GetActor()->IsRootComponentStatic() || Impact.GetActor()->IsRootComponentStationary()) //not really significant if it's just environment
				ProcessInstantHit_Confirmed(Impact, origin, ShootDir, RandomSeed, ReticleSpread);
			else
			{
				//get the component bounding box
				const FBox hitBox = Impact.GetActor()->GetComponentsBoundingBox();

				//calculate that box's extent
				FVector boxExtent = 0.5f * (hitBox.Max - hitBox.Min);
				boxExtent *= weaponConfig.clientSideHitLeeway;

				//avoid precision errors with really thin objects
				boxExtent.X = FMath::Max(20.f, boxExtent.X);
				boxExtent.Y = FMath::Max(20.f, boxExtent.Y);
				boxExtent.Z = FMath::Max(20.f, boxExtent.Z);

				//get the box center
				const FVector boxCenter = (hitBox.Min + hitBox.Max) * 0.5f;

				//if we are within client tolerance
				if (FMath::Abs(Impact.Location.Z - boxCenter.Z) < boxExtent.Z && FMath::Abs(Impact.Location.X - boxCenter.X) < boxExtent.X && FMath::Abs(Impact.Location.Y - boxCenter.Y) < boxExtent.Y)
					ProcessInstantHit_Confirmed(Impact, origin, ShootDir, RandomSeed, ReticleSpread);
				else
					UE_LOG(LogTemp, Log, TEXT("%s Rejected client side hit of %s (outside bounding box tolerance)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
			}
		}
		else if (viewDotHitDir <= weaponConfig.allowedViewDotHitDir)
		{
			UE_LOG(LogTemp, Log, TEXT("%s Rejected client side hit of %s (facing too far from the hit direction)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("%s Rejected client side hit of %s"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		}
	}
}

bool ABulletGunWeapon::ServerNotifyMiss_Validate(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	return true;
}

void ABulletGunWeapon::ServerNotifyMiss_Implementation(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	const FVector origin = GetMuzzleLocation();

	// play FX on remote clients
	hitNotify.origin = origin;
	hitNotify.randomSeed = RandomSeed;
	hitNotify.reticleSpread = ReticleSpread;

	// play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		const FVector EndTrace = origin + ShootDir * weaponConfig.weaponRange;
		SpawnTrailEffect(EndTrace);
	}
}

void ABulletGunWeapon::ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread)
{
	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled() && GetNetMode() == NM_Client)
	{
		//if we're a client and we've hit something that is being controlled by the server
		if (IsValid(Impact.GetActor()) && Impact.GetActor()->GetRemoteRole() == ROLE_Authority)
			ServerNotifyHit(Impact, ShootDir, RandomSeed, ReticleSpread);
		else if (!IsValid(Impact.GetActor()))
		{
			if (Impact.bBlockingHit)
				ServerNotifyHit(Impact, ShootDir, RandomSeed, ReticleSpread);
			else
				ServerNotifyMiss(ShootDir, RandomSeed, ReticleSpread);
		}
	}

	//confirmed hit
	ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
}

void ABulletGunWeapon::ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread)
{
	// handle damage
	if (ShouldDealDamage(Impact.GetActor()))
		DealDamage(Impact, ShootDir);

	// play FX on remote clients
	if (Role == ROLE_Authority)
	{
		hitNotify.origin = Origin;
		hitNotify.randomSeed = RandomSeed;
		hitNotify.reticleSpread = ReticleSpread;
	}

	// play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		const FVector EndTrace = Origin + ShootDir * weaponConfig.weaponRange;
		const FVector EndPoint = Impact.GetActor() ? Impact.ImpactPoint : EndTrace;

		SpawnTrailEffect(EndPoint);
		SpawnImpactEffects(Impact);
	}
}

bool ABulletGunWeapon::ShouldDealDamage(AActor* TestActor) const
{
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (TestActor)
	{
		if (GetNetMode() != NM_Client ||
			TestActor->Role == ROLE_Authority ||
			TestActor->bTearOff)
		{
			return true;
		}
	}

	return false;
}

void ABulletGunWeapon::DealDamage(const FHitResult& Impact, const FVector& ShootDir)
{
	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = weaponConfig.damageType;
	PointDmg.HitInfo = Impact;
	PointDmg.ShotDirection = ShootDir;
	PointDmg.Damage = weaponConfig.hitDamage;

	Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, characterOwner->GetController(), this);
}

void ABulletGunWeapon::OnBurstFinished()
{
	Super::OnBurstFinished();

	currentFiringSpread = 0.0f;
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

void ABulletGunWeapon::OnRep_HitNotify()
{
	SimulateInstantHit(hitNotify.origin, hitNotify.randomSeed, hitNotify.reticleSpread);
}

void ABulletGunWeapon::SimulateInstantHit(const FVector& ShotOrigin, int32 RandomSeed, float ReticleSpread)
{
	FRandomStream WeaponRandomStream(RandomSeed);
	const float ConeHalfAngle = FMath::DegreesToRadians(ReticleSpread * 0.5f);

	const FVector StartTrace = ShotOrigin;
	const FVector AimDir = GetAdjustedAim();
	const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);
	const FVector EndTrace = StartTrace + ShootDir * weaponConfig.weaponRange;

	FHitResult Impact = EquipmentTrace(StartTrace, EndTrace);
	if (Impact.bBlockingHit)
	{
		SpawnImpactEffects(Impact);
		SpawnTrailEffect(Impact.ImpactPoint);
	}
	else
		SpawnTrailEffect(EndTrace);
}

void ABulletGunWeapon::SpawnImpactEffects(const FHitResult& Impact)
{

}

void ABulletGunWeapon::SpawnTrailEffect(const FVector& EndPoint)
{

}

void ABulletGunWeapon::HandleUsing()
{
	//use situations
	if ((currentAmmoInClip > 0 || weaponConfig.bInfiniteClip || weaponConfig.bInfiniteAmmo) && CanUse())
	{
		if (GetNetMode() != NM_DedicatedServer)
			SimulateEquipmentUse();

		if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
		{
			UseEquipment();

			UseAmmo();

			burstCounter++;
		}
	}
	else if (CanReload())
		StartReload();
	else if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (currentAmmo == 0 && !bReusing)
		{
			PlayEquipmentSound(outOfAmmoSound);
			//@TODO: notify HUD of no ammo
		}

		if (burstCounter > 0)
			OnBurstFinished();
	}

	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (Role < ROLE_Authority)
			ServerHandleUsing();

		//reload after firing the last shot
		if (currentAmmoInClip <= 0 && CanReload())
			StartReload();

		bReusing = currentState == EEquipmentState::Using && timesBetweenUse > 0.f;
		if (bReusing)
			GetWorldTimerManager().SetTimer(handleFiringTimer, this, &ABulletGunWeapon::HandleUsing, timesBetweenUse, false);
	}

	lastUseTime = GetWorld()->GetTimeSeconds();
}

void ABulletGunWeapon::OnAltStarted()
{
	if (IsValid(characterOwner))
	{
		characterOwner->GetFollowCamera()->bUsePawnControlRotation = true;
		characterOwner->bUseControllerRotationYaw = true;
		characterOwner->GetCharacterMovement()->bOrientRotationToMovement = false;
		characterOwner->GetCameraBoom()->TargetArmLength *= weaponConfig.aimingScale;
	}
}

void ABulletGunWeapon::OnAltFinished()
{
	if (IsValid(characterOwner))
	{
		characterOwner->GetFollowCamera()->bUsePawnControlRotation = false;
		characterOwner->bUseControllerRotationYaw = false;
		characterOwner->GetCharacterMovement()->bOrientRotationToMovement = true;
		characterOwner->GetCameraBoom()->TargetArmLength /= weaponConfig.aimingScale;
	}
}

void ABulletGunWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABulletGunWeapon, currentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABulletGunWeapon, currentAmmoInClip, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABulletGunWeapon, hitNotify, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(ABulletGunWeapon, bPendingReload, COND_SkipOwner);
}