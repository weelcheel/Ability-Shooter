#include "AbilityShooter.h"
#include "Ability.h"
#include "AbilityShooterCharacter.h"
#include "Projectile.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"

AAbility::AAbility()
{
	//initialize the cooldown values
	currentState = EAbilityState::NoOwner;
	bAutoPerform = true;
	bUltimateAbility = false;

	bWantsToPerform = false;
	bWantsToCooldown = false;
	
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

bool AAbility::ServerHandlePerform_Validate()
{
	return true;
}

void AAbility::ServerHandlePerform_Implementation()
{
	HandlePerform();
}

void AAbility::HandlePerform()
{
	characterOwner->GetFollowCamera()->bUsePawnControlRotation = true;
	characterOwner->bUseControllerRotationYaw = true;
	characterOwner->GetCharacterMovement()->bOrientRotationToMovement = false;

	if (CanPerform())
	{
		if (GetNetMode() != NM_DedicatedServer)
			StartPerformEffects();

		if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
		{
			Perform();
		}
	}
	else if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		OnStopPerform();
	}

	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (Role < ROLE_Authority)
			ServerHandlePerform();
	}
}

void AAbility::OnStopPerform()
{
	characterOwner->GetFollowCamera()->bUsePawnControlRotation = false;
	characterOwner->bUseControllerRotationYaw = false;
	characterOwner->GetCharacterMovement()->bOrientRotationToMovement = true;

	if (GetNetMode() != NM_DedicatedServer)
		StopPerformEffects();
}

void AAbility::DetermineState()
{
	EAbilityState newState = EAbilityState::Idle;

	if (currentState == EAbilityState::OnCooldown || bWantsToCooldown)
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
		HandlePerform();

	if (bWantsToCooldown && currentState == EAbilityState::OnCooldown)
		bWantsToCooldown = false;
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

bool AAbility::ServerStartCooldown_Validate(float manualCooldown /* = -1.f */, EAbilityState cdfState /* = EAbilityState::Idle */)
{
	return true;
}

void AAbility::ServerStartCooldown_Implementation(float manualCooldown /* = -1.f */, EAbilityState cdfState /* = EAbilityState::Idle */)
{
	StartCooldown(manualCooldown, cdfState);
}

void AAbility::StartCooldown_Implementation(float manualCooldown /* = -1.f */, EAbilityState cdfState /* = EAbilityState::Idle */)
{
	bWantsToCooldown = true;
	afterCooldownState = cdfState;
	DetermineState();

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
	SetState(afterCooldownState);
}

FHitResult AAbility::AbilityTrace(const FVector& from, const FVector& to) const
{
	static FName abilityFireTag = FName(TEXT("abilityTrace"));

	FCollisionQueryParams traceParams(abilityFireTag, true, characterOwner);
	traceParams.bTraceAsyncScene = true;
	traceParams.bReturnPhysicalMaterial = true;

	FHitResult hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(hit, from, to, COLLISION_WEAPON, traceParams);

	return hit;
}

FVector AAbility::GetAdjustedAim() const
{
	APlayerController* const pc = IsValid(characterOwner) ? Cast<APlayerController>(characterOwner->GetController()) : nullptr;
	FVector finalAim = FVector::ZeroVector;

	if (IsValid(pc))
	{
		FVector camLoc;
		FRotator camRot;
		pc->GetPlayerViewPoint(camLoc, camRot);
		finalAim = camRot.Vector();
	}
	else if (IsValid(Instigator))
	{
		/*@TODO: implement aim for AI bots */
		finalAim = Instigator->GetBaseAimRotation().Vector();
	}

	return finalAim;
}

FVector AAbility::GetCameraDamageStartLocation(const FVector& aimDir) const
{
	APlayerController* pc = IsValid(characterOwner) ? Cast<APlayerController>(characterOwner->GetController()) : nullptr;
	//@TODO: AI bot aiming
	FVector outStartTrace = FVector::ZeroVector;

	if (IsValid(pc))
	{
		FRotator unusedRot;
		pc->GetPlayerViewPoint(outStartTrace, unusedRot);

		//outStartTrace = outStartTrace + aimDir * ((characterOwner->GetActorLocation() - outStartTrace) | aimDir);
	}

	return outStartTrace;
}

void AAbility::LaunchProjectile(const FName& originSocket, TSubclassOf<AProjectile> projectileType)
{
	USkeletalMeshComponent* useMesh = characterOwner->GetMesh();
	FVector Origin = useMesh->GetSocketLocation(originSocket) + characterOwner->GetActorForwardVector() * 100.f;

	APlayerController* pc = Cast<APlayerController>(characterOwner->GetController());
	FVector camLoc;
	FRotator camRot;
	pc->GetPlayerViewPoint(camLoc, camRot);

	// trace from camera to check what's under crosshair
	const float ProjectileAdjustRange = 10000.0f;
	FVector EndTrace = camLoc + camRot.Vector() * ProjectileAdjustRange;

	FHitResult Impact = AbilityTrace(camLoc, EndTrace);
	FVector shootDir = GetAdjustedAim();

	// and adjust directions to hit that actor
	if (Impact.bBlockingHit)
	{
		FHitResult newImpact = AbilityTrace(Origin, Impact.Location);
		shootDir = (newImpact.Location - Origin).GetSafeNormal();
		DrawDebugLine(GetWorld(), Origin, Origin + shootDir * 1000.f, FColor::Yellow, true, 5.f, 0, 0.8f);
		DrawDebugLine(GetWorld(), Origin, newImpact.Location, FColor::Green, true, 5.f, 0, 0.5f);
	}

	ServerLaunchProjectile(Origin, shootDir, projectileType);
}

bool AAbility::ServerLaunchProjectile_Validate(const FVector& origin, const FVector& launchDir, TSubclassOf<AProjectile> projectileType)
{
	return true;
}

void AAbility::ServerLaunchProjectile_Implementation(const FVector& origin, const FVector& launchDir, TSubclassOf<AProjectile> projectileType)
{
	FTransform projTrans(launchDir.Rotation(), origin);
	FVector realLaunchDir = launchDir.Rotation().Vector();
	AProjectile* projectile = Cast<AProjectile>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, projectileType, projTrans));
	if (projectile)
	{
		projectile->Instigator = characterOwner;
		projectile->SetOwner(this);
		projectile->InitVelocity(realLaunchDir);

		UGameplayStatics::FinishSpawningActor(projectile, projTrans);
	}
}

void AAbility::OnRep_CharacterOwner()
{
	DetermineState();
}

void AAbility::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbility, characterOwner);
	DOREPLIFETIME(AAbility, veteranLevel);
}