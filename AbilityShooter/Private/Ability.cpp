#include "AbilityShooter.h"
#include "Ability.h"
#include "Projectile.h"
#include "DrawDebugHelpers.h"
#include "EquipmentItem.h"
#include "UnrealNetwork.h"
#include "AbilityShooterGameMode.h"
#include "ASPlayerState.h"

AAbility::AAbility()
{
	//initialize the cooldown values
	currentState = EAbilityState::NoOwner;
	bAutoPerform = true;
	bUltimateAbility = false;

	bWantsToPerform = false;
	bWantsToCooldown = false;
	bHasAimingState = false;
	bPerformingRotatesOwnerWithAim = true;
	bStopPerformingWhenButtonIsReleased = false;
	bIgnoreLookInputWhilePerforming = false;
	bIgnoreMovementWhilePerforming = false;

	bIsPerforming = false;
	bIsAiming = false;
	bIsDisabled = false;

	afterCooldownState = EAbilityState::Idle;
	manualCooldownTime = -1.f;
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
}

bool AAbility::CanPerform() const
{
	bool bStateOK = !bIsDisabled && currentState != EAbilityState::NoOwner && currentState != EAbilityState::OnCooldown && IsValid(characterOwner) && characterOwner->CanPerformAbilities();
	return bStateOK;
}

void AAbility::StartPerform()
{
	if (currentState == EAbilityState::Performing)
	{
		HandleInterrupt(EAbilityInterruptSignal::UserCancelled);
		return;
	}
	else if (currentState == EAbilityState::OnCooldown || bIsDisabled || !characterOwner->CanPerformAbilities())
		return;

	if (Role < ROLE_Authority)
	{
		ServerStartPerform();
	}

	if (!bWantsToPerform)
	{
		bWantsToPerform = true;
		DetermineState();
	}
}

void AAbility::StopPerform(bool bFromInput)
{
	if (bFromInput && !bStopPerformingWhenButtonIsReleased)
	{
		bWantsToPerform = false;
		return;
	}

	if (Role < ROLE_Authority)
		ServerStopPerform(bFromInput);

	if (bWantsToPerform)
	{
		bWantsToPerform = false;
		DetermineState();
	}	
}

void AAbility::ConfirmAim()
{
	if (Role < ROLE_Authority)
		ServerConfirmAim();

	if (!bWantsToConfirmAim)
	{
		bWantsToConfirmAim = true;
		DetermineState();
	}
}

void AAbility::ForceStopAbility_Implementation()
{
	//only force the stop if the ability is already performing
	if (currentState != EAbilityState::Performing)
		return;

	bWantsToPerform = false;
	OnStopPerform(true, true);

	if (GetWorldTimerManager().GetTimerRemaining(executionTimer) > 0.f && IsValid(characterOwner))
	{
		characterOwner->ForceEndCurrentAction();
		GetWorldTimerManager().ClearTimer(executionTimer);
	}
}

void AAbility::StopAbility_Implementation()
{
	//only stop if the ability is already performing
	if (currentState != EAbilityState::Performing)
		return;

	bWantsToPerform = false;
	OnStopPerform();

	if (GetWorldTimerManager().GetTimerRemaining(executionTimer) > 0.f && IsValid(characterOwner))
	{
		characterOwner->ForceEndCurrentAction();
		GetWorldTimerManager().ClearTimer(executionTimer);
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

bool AAbility::ServerStopPerform_Validate(bool bFromInput)
{
	return true;
}

void AAbility::ServerStopPerform_Implementation(bool bFromInput)
{
	StopPerform(bFromInput);
}

bool AAbility::ServerHandlePerform_Validate()
{
	return true;
}

void AAbility::ServerHandlePerform_Implementation()
{
	HandlePerform();
}

bool AAbility::ServerConfirmAim_Validate()
{
	return true;
}

void AAbility::ServerConfirmAim_Implementation()
{
	ConfirmAim();
}

bool AAbility::ServerReceiveInterrupt_Validate(EAbilityInterruptSignal signal)
{
	return true;
}

void AAbility::ServerReceiveInterrupt_Implementation(EAbilityInterruptSignal signal)
{
	AbilityReceivedInterruptSignal(signal);
}

void AAbility::HandleInterrupt(EAbilityInterruptSignal signal)
{
	if (currentState == EAbilityState::Aiming)
	{
		SetState(EAbilityState::Idle);
		if (!HasAuthority())
			ServerReceiveInterrupt(signal);

		return;
	}

	if (HasAuthority())
		AbilityReceivedInterruptSignal(signal);
	else
		ServerReceiveInterrupt(signal);
}

void AAbility::HandlePerform()
{
	if (CanPerform())
	{
		if (!bAutoPerform)
		{
			ShouldPerform();
		}
		else
			ContinueHandlePerform();
	}
	else if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		OnStopPerform(false);
	}

	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (Role < ROLE_Authority)
			ServerHandlePerform();
	}
}

void AAbility::ContinueHandlePerform()
{
	if (bShouldStopAllOtherAbilitiesOnUse && IsValid(characterOwner))
	{
		for (AAbility* ability : characterOwner->abilities)
		{
			if (!IsValid(ability))
				continue;

			if ((ability->GetCurrentState() == EAbilityState::Performing || ability->GetCurrentState() == EAbilityState::Aiming) && ability != this)
				ability->ForceStopAbility();
		}
	}

	if (bShouldDisableAllOtherAbilitiesOnUse && IsValid(characterOwner))
	{
		for (AAbility* ability : characterOwner->abilities)
		{
			if (!IsValid(ability))
				continue;

			if (ability != this)
				ability->SetDisabled(true);
		}
	}

	if (bPerformingRotatesOwnerWithAim)
	{
		characterOwner->GetFollowCamera()->bUsePawnControlRotation = true;
		characterOwner->bUseControllerRotationYaw = true;
		characterOwner->GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	if (bIgnoreMovementWhilePerforming && IsValid(characterOwner))
		characterOwner->GetCharacterMovement()->SetMovementMode(MOVE_None);

	if (GetNetMode() != NM_DedicatedServer)
		StartPerformEffects();

	bIsPerforming = true;

	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (bIgnoreLookInputWhilePerforming && IsValid(characterOwner) && IsValid(characterOwner->GetController()))
		{
			APlayerController* pc = Cast<APlayerController>(characterOwner->GetController());
			if (IsValid(pc))
				pc->SetIgnoreLookInput(true);
		}

		if (executionTimeInfo.duration > 0.f)
		{
			ServerStartExecutionTimer();
			GetWorldTimerManager().SetTimer(executionTimer, this, &AAbility::Perform, executionTimeInfo.duration);
		}
		else
			Perform();
	}
}

void AAbility::StopHandlePerform()
{
	//stop the ability before it actually starts
	currentState = EAbilityState::Idle;

	bWantsToPerform = false;
	OnStopPerform(false);
}

void AAbility::OnStopPerform(bool bFromPerforming, bool bForcedStop)
{
	if (!bFromPerforming) //if we performed before this or if the ability is just failing to start
	{
		bIsPerforming = false;
		return;
	}

	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (bIgnoreLookInputWhilePerforming && IsValid(characterOwner) && IsValid(characterOwner->GetController()))
		{
			APlayerController* pc = Cast<APlayerController>(characterOwner->GetController());
			if (IsValid(pc))
				pc->SetIgnoreLookInput(false);
		}

		OnAbilityStopped();
	}

	if (IsValid(characterOwner))
	{
		if (bPerformingRotatesOwnerWithAim)
		{
			characterOwner->GetFollowCamera()->bUsePawnControlRotation = false;
			characterOwner->bUseControllerRotationYaw = false;
			characterOwner->GetCharacterMovement()->bOrientRotationToMovement = true;
		}
	}

	if (bIgnoreMovementWhilePerforming && IsValid(characterOwner))
		characterOwner->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (GetNetMode() != NM_DedicatedServer)
		StopPerformEffects();

	if (HasAuthority())
	{
		if (bForcedStop && (veteranLevel >= 0 && veteranLevel < interruptCooldown.Num()))
		{
			StartCooldown(interruptCooldown[veteranLevel]);
		}
		else
			StartCooldown();
	}
		

	if (bShouldDisableAllOtherAbilitiesOnUse && IsValid(characterOwner))
	{
		for (AAbility* ability : characterOwner->abilities)
		{
			if (!IsValid(ability))
				continue;

			if (ability != this)
				ability->SetDisabled(false);
		}
	}

	bIsPerforming = false;
}

void AAbility::DetermineState()
{
	EAbilityState newState = EAbilityState::Idle;

	if (currentState == EAbilityState::OnCooldown || bWantsToCooldown || GetWorldTimerManager().GetTimerRemaining(cooldownTimer) > 0.f)
		newState = EAbilityState::OnCooldown;
	else if (IsValid(characterOwner))
	{
		if (bWantsToPerform && CanPerform() && characterOwner->CanPerformAbilities() && currentState != EAbilityState::Aiming)
		{
			//stop equipment use
			AEquipmentItem* equipment = characterOwner->GetCurrentEquipment();
			if (IsValid(equipment))
			{
				equipment->StopUse();
				equipment->StopAlt();
			}

			//either aim if we need to or just go straight to performing
			if (!characterOwner->ShouldQuickAimAbilities() && bHasAimingState)
				newState = EAbilityState::Aiming;
			else
				newState = EAbilityState::Performing;
		}
		else if (bWantsToConfirmAim && CanPerform() && characterOwner->CanPerformAbilities() && currentState == EAbilityState::Aiming)
		{
			newState = EAbilityState::Performing;
			bWantsToConfirmAim = false;
		}
			
	}
	else
		newState = EAbilityState::NoOwner;

	SetState(newState);
}

void AAbility::SetState(EAbilityState newState)
{
	const EAbilityState prevState = currentState;
	currentState = newState;

	if (prevState != EAbilityState::OnCooldown && newState == EAbilityState::OnCooldown)
	{
		bWantsToCooldown = false;
		StartCooldownTimer();

		return;
	}

	if (prevState == EAbilityState::Performing && newState != EAbilityState::Performing)
		OnStopPerform();
	else if (prevState == EAbilityState::Aiming && newState != EAbilityState::Aiming)
	{
		bIsAiming = false;
		OnAimingStopped();
	}

	if (prevState != EAbilityState::Performing && currentState == EAbilityState::Performing)
		HandlePerform();
	else if (prevState != EAbilityState::Aiming && currentState == EAbilityState::Aiming)
	{
		bIsAiming = true;
		OnAimingStarted();
	}
}

void AAbility::SetupAbility(AAbilityShooterCharacter* newOwner)
{
	if (Role < ROLE_Authority)
		return;

	if (IsValid(newOwner))
	{
		SetOwner(newOwner);
		FAttachmentTransformRules rules(EAttachmentRule::SnapToTarget, false);
		AttachToActor(newOwner, rules);

		characterOwner = newOwner;
	}
	else
	{
		characterOwner = nullptr;

		FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, false);
		DetachFromActor(rules);
	}

	if (IsValid(characterOwner))
	{
		characterOwner->OnShooterDamaged.AddDynamic(this, &AAbility::OnOwnerDamaged);
		characterOwner->OnShooterDealtDamage.AddDynamic(this, &AAbility::OnOwnerDealtDamage);
	}
	
	DetermineState();
}

void AAbility::SetDisabled(bool bDisabled /* = false */)
{
	bIsDisabled = bDisabled;
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
	manualCooldownTime = manualCooldown;
	DetermineState();
}

void AAbility::StartCooldownTimer()
{
	float duration;
	if (manualCooldownTime == 0.f)
	{
		currentState = afterCooldownState;
		return;
	}
	else
		duration = manualCooldownTime > 0.f ? manualCooldownTime : GetVeteranLevelScaledValue(baseCooldownTimes);

	//@TODO: let cooldown reduction modify duration
	GetWorldTimerManager().SetTimer(cooldownTimer, this, &AAbility::CooldownFinished, duration);
}

void AAbility::CooldownFinished()
{
	if (currentState != EAbilityState::NoOwner)
		SetState(afterCooldownState);

	afterCooldownState = EAbilityState::Idle;
	manualCooldownTime = -1.f;
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

void AAbility::LaunchProjectile(const FName& originSocket, TSubclassOf<AProjectile> projectileType, const TArray<FEffectStatAlter>& projectileEffectAlters, float damage)
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

	//FHitResult Impact = AbilityTrace(camLoc, EndTrace);
	FVector shootDir = GetAdjustedAim();

	// and adjust directions to hit that actor
	/*if (Impact.bBlockingHit)
	{
		FHitResult newImpact = AbilityTrace(Origin, Impact.Location);
		shootDir = (newImpact.Location - Origin).GetSafeNormal();
		DrawDebugLine(GetWorld(), Origin, Origin + shootDir * 1000.f, FColor::Yellow, true, 5.f, 0, 0.8f);
		DrawDebugLine(GetWorld(), Origin, newImpact.Location, FColor::Green, true, 5.f, 0, 0.5f);
	}*/

	ServerLaunchProjectile(Origin, shootDir, projectileType, projectileEffectAlters, damage);
}

bool AAbility::ServerLaunchProjectile_Validate(const FVector& origin, const FVector& launchDir, TSubclassOf<AProjectile> projectileType, const TArray<FEffectStatAlter>& projectileEffectAlters, float damage)
{
	return true;
}

void AAbility::ServerLaunchProjectile_Implementation(const FVector& origin, const FVector& launchDir, TSubclassOf<AProjectile> projectileType, const TArray<FEffectStatAlter>& projectileEffectAlters, float damage)
{
	FTransform projTrans(launchDir.Rotation(), origin);
	FVector realLaunchDir = launchDir.Rotation().Vector();
	AProjectile* projectile = Cast<AProjectile>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, projectileType, projTrans));
	if (projectile)
	{
		projectile->Instigator = characterOwner;
		projectile->SetOwner(this);
		projectile->InitVelocity(realLaunchDir);
		projectile->impactEffectStatAlters = projectileEffectAlters;
		projectile->projConfig.explosionDamage = damage;

		UGameplayStatics::FinishSpawningActor(projectile, projTrans);
	}
}

bool AAbility::ServerStartExecutionTimer_Validate()
{
	return true;
}

void AAbility::ServerStartExecutionTimer_Implementation()
{
	if (IsValid(characterOwner))
		characterOwner->AllApplyAction(executionTimeInfo);
}

void AAbility::ConeTrace(FVector& start, FVector& end, float boxSize, TArray<FHitResult>& outHits)
{
	TArray<FHitResult> hits;
	FVector halfSize(boxSize, boxSize, boxSize);

	FCollisionShape box = FCollisionShape::MakeBox(halfSize * 0.5f);
	GetWorld()->SweepMultiByChannel(hits, start, end, FRotator::ZeroRotator.Quaternion(), COLLISION_WEAPON, box);

	for (int32 i = 0; i < hits.Num(); i++)
	{
		FVector coneNormal = (hits[i].ImpactPoint - start).GetSafeNormal();
		float objAngle = FMath::Acos(FVector::DotProduct(coneNormal, start));
		float coneMaxAngle = FMath::Tan(boxSize / (start - end).Size());

		if (objAngle > coneMaxAngle)
			hits.RemoveAt(i);
	}

	outHits = hits;
}

void AAbility::OnRep_CharacterOwner()
{
	DetermineState();
}

void AAbility::OnRep_IsPerforming()
{
	if (bIsPerforming)
	{
		if (!bAutoPerform)
		{
			ShouldPerform();
		}
		else
			ContinueHandlePerform();
	}
	else
		OnStopPerform();
}

void AAbility::OnRep_IsAiming()
{
	if (bIsAiming)
		OnAimingStarted();
	else
		OnAimingStopped();
}

bool AAbility::CanHurtCharacter(AAbilityShooterCharacter* testCharacter) const
{
	AAbilityShooterGameMode* gm = GetWorld()->GetAuthGameMode<AAbilityShooterGameMode>();

	if (!IsValid(characterOwner) || !IsValid(testCharacter))
		return false;
	if (!testCharacter->IsAlive())
		return false;
	
	return IsValid(gm) ? gm->CanDealDamage(Cast<AASPlayerState>(characterOwner->PlayerState), Cast<AASPlayerState>(testCharacter->PlayerState)) : false;
}

void AAbility::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbility, characterOwner);
	DOREPLIFETIME(AAbility, veteranLevel);
	DOREPLIFETIME(AAbility, bIsPerforming);
}