#include "AbilityShooter.h"
#include "EquipmentItem.h"
#include "Sound/SoundCue.h"
#include "UnrealNetwork.h"
#include "AbilityShooterCharacter.h"
#include "Animation/AnimMontage.h"
#include "Ability.h"

AEquipmentItem::AEquipmentItem()
{
	mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EquipmentMesh"));
	mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	mesh->CastShadow = true;
	mesh->SetCollisionObjectType(ECC_WorldDynamic);
	mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	mesh->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	mesh->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	RootComponent = mesh;

	bPlayingUseAnim = false;
	bIsEquipped = false;
	bWantsToUse = false;
	bPendingEquip = false;
	currentState = EEquipmentState::NoOwner;
	bWantsToAlt = false;
	bIsAltActive = false;

	burstCounter = 0;
	lastUseTime = 0.0f;

	Tags.Add(TEXT("usable"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

void AEquipmentItem::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	DetachMeshFromCharacter();
}

void AEquipmentItem::Destroyed()
{
	Super::Destroyed();

	StopSimulatingEquipmentUse();
}

void AEquipmentItem::OnEquip(const AEquipmentItem* lastItem)
{
	AttachMeshToCharacter();

	bPendingEquip = true;
	DetermineEquipmentState();

	if (IsValid(lastItem))
	{
		float duration = PlayEquipmentAnimation(equipAnim);
		if (duration <= 0.f)
			duration = 0.5f;

		equipStartedTime = GetWorld()->GetTimeSeconds();
		equipDuration = duration;

		GetWorldTimerManager().SetTimer(onEquipFinishedTimer, this, &AEquipmentItem::OnEquipFinished, duration, false);
	}
	else
		OnEquipFinished();

	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
		PlayEquipmentSound(equipSound);

	if (IsValid(characterOwner))
	{
		//characterOwner->OnShooterDamaged.BindDynamic(this, &AEquipmentItem::OnOwnerDamaged);
		//characterOwner->OnShooterDealtDamage.BindDynamic(this, &AEquipmentItem::OnOwnerDealtDamage);

		FShooterDamagedDelegate damageEvent;
		damageEvent.BindUObject(this, &AEquipmentItem::OnOwnerDamaged);
		characterOwner->OnShooterDamagedEvents.Add(damageEvent);

		FShooterDealtDamageDelegate damagedEvent;
		damagedEvent.BindUObject(this, &AEquipmentItem::OnOwnerDealtDamage);
		characterOwner->OnShooterDealtDamageEvents.Add(damagedEvent);
	}
}

void AEquipmentItem::OnEquipFinished()
{
	AttachMeshToCharacter();

	bIsEquipped = true;
	bPendingEquip = false;

	DetermineEquipmentState();
}

void AEquipmentItem::OnUnEquip()
{
	DetachMeshFromCharacter();
	bIsEquipped = false;
	StopUse();

	if (bPendingEquip)
	{
		StopEquipmentAnimation(equipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(onEquipFinishedTimer);
	}

	if (IsValid(characterOwner))
	{
		//characterOwner->OnShooterDamaged.RemoveAll(this);
		//characterOwner->OnShooterDealtDamage.RemoveAll(this);
	}

	DetermineEquipmentState();
}

void AEquipmentItem::OnEnterInventory(AAbilityShooterCharacter* newOwner)
{
	SetOwningCharacter(newOwner);

	currentState = EEquipmentState::Idle;
}

void AEquipmentItem::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
		SetOwningCharacter(nullptr);

	if (IsAttachedToCharacter())
		OnUnEquip();

	currentState = EEquipmentState::NoOwner;
}

void AEquipmentItem::AttachMeshToCharacter()
{
	if (IsValid(characterOwner))
	{
		DetachMeshFromCharacter();
		mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		mesh->SetSimulatePhysics(false);
		mesh->PutRigidBodyToSleep();

		FName attachPoint = characterOwner->GetEquipmentAttachPoint();
		USkeletalMeshComponent* charMesh = characterOwner->GetMesh();
		mesh->SetHiddenInGame(false);

		FAttachmentTransformRules rules(EAttachmentRule::SnapToTarget, false);
		mesh->AttachToComponent(charMesh, rules, attachPoint);
	}
}

void AEquipmentItem::DetachMeshFromCharacter()
{
	if (!IsValid(characterOwner))
	{
		FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, false);
		mesh->DetachFromComponent(rules);

		mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		mesh->SetSimulatePhysics(true);
		mesh->WakeRigidBody();
	}
	else
	{
		FDetachmentTransformRules rules(EDetachmentRule::KeepRelative, false);
		mesh->DetachFromComponent(rules);

		mesh->SetHiddenInGame(true);
	}
}

void AEquipmentItem::StartUse()
{
	if (Role < ROLE_Authority)
		ServerStartUse();

	if (!bWantsToUse)
	{
		bWantsToUse = true;
		DetermineEquipmentState();
	}

	if (HasAuthority() && IsValid(characterOwner))
		characterOwner->SendInterruptToAbilities(EAbilityInterruptSignal::EquipementUsed);
}

void AEquipmentItem::StopUse()
{
	if (Role < ROLE_Authority)
		ServerStopUse();

	if (bWantsToUse)
	{
		bWantsToUse = false;
		DetermineEquipmentState();
	}
}

void AEquipmentItem::StartAlt()
{
	if (!IsValid(characterOwner))
		return;

	if (Role < ROLE_Authority)
		ServerStartAlt();

	if (!bWantsToAlt && characterOwner->CanUseEquipment())
	{
		bWantsToAlt = true;
		if (!bIsAltActive)
			OnAltStarted();
	}

	if (HasAuthority() && IsValid(characterOwner))
		characterOwner->SendInterruptToAbilities(EAbilityInterruptSignal::EquipementUsed);
}

void AEquipmentItem::StopAlt()
{
	if (Role < ROLE_Authority)
		ServerStopAlt();

	if (bWantsToAlt)
	{
		bWantsToAlt = false;
		if (bIsAltActive)
			OnAltFinished();
	}
}

bool AEquipmentItem::ServerStartUse_Validate()
{
	return true;
}

void AEquipmentItem::ServerStartUse_Implementation()
{
	StartUse();
}

bool AEquipmentItem::ServerStopUse_Validate()
{
	return true;
}

void AEquipmentItem::ServerStopUse_Implementation()
{
	StopUse();
}

bool AEquipmentItem::ServerStartAlt_Validate()
{
	return true;
}

void AEquipmentItem::ServerStartAlt_Implementation()
{
	StartAlt();
}

bool AEquipmentItem::ServerStopAlt_Validate()
{
	return true;
}

void AEquipmentItem::ServerStopAlt_Implementation()
{
	StopAlt();
}

bool AEquipmentItem::CanUse() const
{
	bool bCanUse = IsValid(characterOwner) && characterOwner->CanUseEquipment();
	bool bStateOk = currentState == EEquipmentState::Idle || currentState == EEquipmentState::Using;
	return bCanUse && bStateOk;
}

void AEquipmentItem::HandleUsing()
{
	//use situations
	if (CanUse())
	{
		if (GetNetMode() != NM_DedicatedServer)
			SimulateEquipmentUse();

		if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
		{
			UseEquipment();
			OnUse();

			burstCounter++;
		}
	}
	else if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (burstCounter > 0)
			OnBurstFinished();
	}

	if (IsValid(characterOwner) && characterOwner->IsLocallyControlled())
	{
		if (Role < ROLE_Authority)
			ServerHandleUsing();

		bReusing = currentState == EEquipmentState::Using && characterOwner->GetCurrentStat(EStat::ES_EquipUseRate) > 0.f;
		if (bReusing)
			GetWorldTimerManager().SetTimer(handleFiringTimer, this, &AEquipmentItem::HandleUsing, characterOwner->GetCurrentStat(EStat::ES_EquipUseRate), false);
	}

	lastUseTime = GetWorld()->GetTimeSeconds();
}

bool AEquipmentItem::ServerHandleUsing_Validate()
{
	return true;
}

void AEquipmentItem::ServerHandleUsing_Implementation()
{
	HandleUsing();
}

void AEquipmentItem::SetEquipmentState(EEquipmentState newState)
{
	const EEquipmentState prevState = currentState;

	if (prevState == EEquipmentState::Using && newState != EEquipmentState::Using)
		OnBurstFinished();

	currentState = newState;

	if (prevState != EEquipmentState::Using && newState == EEquipmentState::Using)
		OnBurstStarted();
}

void AEquipmentItem::DetermineEquipmentState()
{
	EEquipmentState newState = EEquipmentState::Idle;

	if (bIsEquipped)
	{
		if (bWantsToUse && CanUse())
			newState = EEquipmentState::Using;
	}
	else if (bPendingEquip)
		newState = EEquipmentState::Equipping;

	SetEquipmentState(newState);
}

void AEquipmentItem::OnBurstStarted()
{
	characterOwner->GetFollowCamera()->bUsePawnControlRotation = true;
	characterOwner->bUseControllerRotationYaw = true;
	characterOwner->GetCharacterMovement()->bOrientRotationToMovement = false;

	const float gameTime = GetWorld()->GetTimeSeconds();
	if (lastUseTime > 0.f && timesBetweenUse > 0.f && lastUseTime + timesBetweenUse > gameTime)
		GetWorldTimerManager().SetTimer(handleFiringTimer, this, &AEquipmentItem::HandleUsing, lastUseTime + timesBetweenUse - gameTime, false);
	else
		HandleUsing();
}

void AEquipmentItem::OnBurstFinished()
{
	if (!bIsAltActive)
	{
		characterOwner->GetFollowCamera()->bUsePawnControlRotation = false;
		characterOwner->bUseControllerRotationYaw = false;
		characterOwner->GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	burstCounter = 0;

	if (GetNetMode() != NM_DedicatedServer)
		StopSimulatingEquipmentUse();

	GetWorldTimerManager().ClearTimer(handleFiringTimer);
	bReusing = false;

	OnUseEnded();
}

void AEquipmentItem::OnAltStarted()
{
	if (IsValid(characterOwner))
		characterOwner->bWantsToUseAlt = true;
	if (IsValid(characterOwner) && !characterOwner->CanUseEquipment())
		return;

	bIsAltActive = true;

	UseAltStarted();
}

void AEquipmentItem::OnAltFinished()
{
	if (IsValid(characterOwner))
		characterOwner->bWantsToUseAlt = false;

	bIsAltActive = false;

	UseAltStopped();
}

UAudioComponent* AEquipmentItem::PlayEquipmentSound(USoundCue* sound)
{
	UAudioComponent* ac = nullptr;
	if (IsValid(sound) && IsValid(characterOwner))
		ac = UGameplayStatics::SpawnSoundAttached(sound, characterOwner->GetRootComponent());

	return ac;
}

float AEquipmentItem::PlayEquipmentAnimation(UAnimMontage* animation)
{
	float duration = 0.f;
	if (IsValid(characterOwner) && IsValid(animation))
		duration = characterOwner->PlayAnimMontage(animation);
	
	return duration;
}

void AEquipmentItem::StopEquipmentAnimation(UAnimMontage* animation)
{
	if (IsValid(characterOwner) && IsValid(animation))
		characterOwner->StopAnimMontage(animation);
}

FVector AEquipmentItem::GetCameraAim() const
{
	APlayerController* const pc = IsValid(Instigator) ? Cast<APlayerController>(Instigator->Controller) : nullptr;
	FVector finalAim = FVector::ZeroVector;

	if (IsValid(pc))
	{
		FVector camLoc;
		FRotator camRot;
		pc->GetPlayerViewPoint(camLoc, camRot);
		finalAim = camRot.Vector();
	}
	else if (IsValid(Instigator))
		finalAim = Instigator->GetBaseAimRotation().Vector();

	return finalAim;
}

FVector AEquipmentItem::GetAdjustedAim() const
{
	APlayerController* const pc = IsValid(Instigator) ? Cast<APlayerController>(Instigator->GetController()) : nullptr;
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

FVector AEquipmentItem::GetCameraDamageStartLocation(const FVector& AimDir) const 
{
	APlayerController* pc = IsValid(characterOwner) ? Cast<APlayerController>(characterOwner->GetController()) : nullptr;
	//@TODO: AI bot aiming
	FVector outStartTrace = FVector::ZeroVector;

	if (IsValid(pc))
	{
		FRotator unusedRot;
		pc->GetPlayerViewPoint(outStartTrace, unusedRot);
		
		outStartTrace = outStartTrace + AimDir * ((Instigator->GetActorLocation() - outStartTrace) | AimDir);
	}

	return outStartTrace;
}

FHitResult AEquipmentItem::EquipmentTrace(const FVector& TraceFrom, const FVector& TraceTo) const
{
	static FName weaponFireTag = FName(TEXT("weaponTrace"));

	FCollisionQueryParams traceParams(weaponFireTag, true, Instigator);
	traceParams.bTraceAsyncScene = true;
	traceParams.bReturnPhysicalMaterial = true;

	FHitResult hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(hit, TraceFrom, TraceTo, COLLISION_WEAPON, traceParams);

	return hit;
}

void AEquipmentItem::SetOwningCharacter(AAbilityShooterCharacter* newOwner)
{
	if (characterOwner != newOwner)
	{
		Instigator = newOwner;
		characterOwner = newOwner;
		SetOwner(newOwner);
	}
}

void AEquipmentItem::OnRep_characterOwner()
{
	if (IsValid(characterOwner))
		OnEnterInventory(characterOwner);
	else
		OnLeaveInventory();
}

void AEquipmentItem::OnRep_BurstCounter()
{
	if (burstCounter > 0)
		SimulateEquipmentUse();
	else
		StopSimulatingEquipmentUse();
}

void AEquipmentItem::OnRep_AltToggle()
{
	if (bWantsToAlt)
		OnAltStarted();
	else
		OnAltFinished();
}

void AEquipmentItem::SimulateEquipmentUse()
{
	if (Role == ROLE_Authority && currentState != EEquipmentState::Using)
		return;

	//@TODO: equipment FX

	if (bLoopedUseAnim || !bPlayingUseAnim)
	{
		PlayEquipmentAnimation(useAnim);
		bPlayingUseAnim = true;
	}

	if (bLoopedUseSound)
	{
		if (!IsValid(usingAC))
			usingAC = PlayEquipmentSound(useLoopSound);
	}
	else
		PlayEquipmentSound(useSound);
}

void AEquipmentItem::StopSimulatingEquipmentUse()
{
	if (bLoopedUseAnim || bPlayingUseAnim)
	{
		StopEquipmentAnimation(useAnim);
		bPlayingUseAnim = false;
	}

	if (IsValid(usingAC))
	{
		usingAC->FadeOut(0.1f, 0.0f);
		usingAC = nullptr;

		PlayEquipmentSound(useFinishSound);
	}
}

void AEquipmentItem::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEquipmentItem, characterOwner);
	DOREPLIFETIME(AEquipmentItem, bWantsToAlt);
	DOREPLIFETIME_CONDITION(AEquipmentItem, burstCounter, COND_SkipOwner);
}

USkeletalMeshComponent* AEquipmentItem::GetMesh() const
{
	return mesh;
}

AAbilityShooterCharacter* AEquipmentItem::GetCharacterOwner() const
{
	return characterOwner;
}

bool AEquipmentItem::IsEquipped() const
{
	return bIsEquipped;
}

bool AEquipmentItem::IsAttachedToCharacter() const
{
	return bIsEquipped || bPendingEquip;
}

EEquipmentState AEquipmentItem::GetCurrentState() const
{
	return currentState;
}

float AEquipmentItem::GetEquipStartedTime() const
{
	return equipStartedTime;
}

float AEquipmentItem::GetEquipDuration() const
{
	return equipDuration;
}

USkeletalMeshComponent* AEquipmentItem::GetEquipmentMesh() const
{
	return mesh;
}

bool AEquipmentItem::IsAltActive() const
{
	return bIsAltActive;
}