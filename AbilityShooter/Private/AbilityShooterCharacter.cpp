#include "AbilityShooter.h"
#include "AbilityShooterCharacter.h"
#include "AbilityShooterGameMode.h"
#include "EquipmentItem.h"
#include "UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// AAbilityShooterCharacter

AAbilityShooterCharacter::AAbilityShooterCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	
	health = 100.f;
}

void AAbilityShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Role == ROLE_Authority)
	{
		health = GetMaxHealth();
		SpawnDefaultInventory();
	}

	//@TODO: setup team color material IDs

	//play respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (IsValid(respawnFX))
			UGameplayStatics::SpawnEmitterAtLocation(this, respawnFX, GetActorLocation(), GetActorRotation());
		if (IsValid(respawnSound))
			UGameplayStatics::PlaySoundAtLocation(this, respawnSound, GetActorLocation());
	}
}

void AAbilityShooterCharacter::Destroyed()
{
	Super::Destroyed();

	DestroyInventory();
}

void AAbilityShooterCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	//reattach equipment if needed
	SetCurrentEquipment(currentEquipment);

	//@TODO: set mesh team color material instance
}

void AAbilityShooterCharacter::PossessedBy(class AController* C)
{
	Super::PossessedBy(C);

	//@TODO: set team color now that we have a player state on the server
}

void AAbilityShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//@TODO: set team color now that we have a player state on the client
}

FRotator AAbilityShooterCharacter::GetAimOffsets() const
{
	const FVector aimDirWorld = GetBaseAimRotation().Vector();
	const FVector aimDirLocal = ActorToWorld().InverseTransformVectorNoScale(aimDirWorld);
	const FRotator aimRotLocal = aimDirLocal.Rotation();

	return aimRotLocal;
}

void AAbilityShooterCharacter::SpawnDefaultInventory()
{
	if (Role < ROLE_Authority)
		return;

	for (TSubclassOf<AEquipmentItem> equipClass : defaultEquipmentList)
	{
		if (equipClass)
		{
			FActorSpawnParameters spawnInfo;
			spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AEquipmentItem* newItem = GetWorld()->SpawnActor<AEquipmentItem>(equipClass, spawnInfo);
			AddEquipment(newItem);
		}
	}

	if (equipmentInventory.Num() > 0)
		EquipEquipment(equipmentInventory[0]);
}

void AAbilityShooterCharacter::DestroyInventory()
{
	if (Role < ROLE_Authority)
		return;

	for (int32 i = 0; i < equipmentInventory.Num(); i++)
	{
		AEquipmentItem* item = equipmentInventory[i];
		if (IsValid(item))
		{
			RemoveEquipment(item);
			item->Destroy();
		}
	}
}

void AAbilityShooterCharacter::AddEquipment(AEquipmentItem* item)
{
	if (IsValid(item) && Role == ROLE_Authority)
	{
		item->OnEnterInventory(this);
		equipmentInventory.AddUnique(item);
	}
}

void AAbilityShooterCharacter::RemoveEquipment(AEquipmentItem* item)
{
	if (IsValid(item) && Role == ROLE_Authority)
	{
		item->OnLeaveInventory();
		equipmentInventory.RemoveSingle(item);
	}
}

AEquipmentItem* AAbilityShooterCharacter::FindEquipment(TSubclassOf<AEquipmentItem> equipmentClass)
{
	for (AEquipmentItem* item : equipmentInventory)
	{
		if (IsValid(item) && item->IsA(equipmentClass))
			return item;
	}

	return nullptr;
}

void AAbilityShooterCharacter::EquipEquipment(AEquipmentItem* item)
{
	if (IsValid(item))
	{
		if (Role == ROLE_Authority)
			SetCurrentEquipment(item);
		else
			ServerEquipEquipment(item);
	}
}

bool AAbilityShooterCharacter::ServerEquipEquipment_Validate(AEquipmentItem* newEquipment)
{
	return true;
}

void AAbilityShooterCharacter::ServerEquipEquipment_Implementation(AEquipmentItem* newEquipment)
{
	EquipEquipment(newEquipment);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AAbilityShooterCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings
	check(InputComponent);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	InputComponent->BindAxis("MoveForward", this, &AAbilityShooterCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AAbilityShooterCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AAbilityShooterCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AAbilityShooterCharacter::LookUpAtRate);

	// handle touch devices
	InputComponent->BindTouch(IE_Pressed, this, &AAbilityShooterCharacter::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &AAbilityShooterCharacter::TouchStopped);

	//current Equipment (weapon or Active Outfit Ability) interaction
	InputComponent->BindAction("Primary", IE_Pressed, this, &AAbilityShooterCharacter::PrimaryUseStart);
	InputComponent->BindAction("Primary", IE_Released, this, &AAbilityShooterCharacter::PrimaryUseStop);
	InputComponent->BindAction("Alternate", IE_Pressed, this, &AAbilityShooterCharacter::AlternateUseStart);
	InputComponent->BindAction("Alternate", IE_Released, this, &AAbilityShooterCharacter::AlternateUseStop);

	//ability interaction
	InputComponent->BindAction("Ability1", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<0>);
	InputComponent->BindAction("Ability1", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<0>);
	InputComponent->BindAction("Ability2", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<1>);
	InputComponent->BindAction("Ability2", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<1>);
	InputComponent->BindAction("Ultimate", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<2>);
	InputComponent->BindAction("Ultimate", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<2>);
}

void AAbilityShooterCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AAbilityShooterCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AAbilityShooterCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			/*UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, false, "Damage");
			}*/
		}
	}

	if (DamageTaken > 0.f)
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);

	/*AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
	if (MyHUD)
	{
		MyHUD->NotifyWeaponHit(DamageTaken, DamageEvent, PawnInstigator);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		AShooterPlayerController* InstigatorPC = Cast<AShooterPlayerController>(PawnInstigator->Controller);
		AShooterHUD* InstigatorHUD = InstigatorPC ? Cast<AShooterHUD>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}*/
}

void AAbilityShooterCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	else
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
}

float AAbilityShooterCharacter::TakeDamage(float Damage, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	//@TODO: check for invulnerability

	//don't damage already dead characters
	if (health <= 0.f)
		return 0.0f;

	//@TODO: let the gametype modify the damage
	//@TODO: let the stats then modify the damage

	//call the super version for blueprint hooks
	const float actualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (actualDamage > 0.f)
	{
		health -= actualDamage;
		if (health <= 0.f)
			Die(actualDamage, DamageEvent, EventInstigator, DamageCauser);
		else
			PlayHit(actualDamage, DamageEvent, IsValid(EventInstigator) ? EventInstigator->GetPawn() : nullptr, DamageCauser);

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return actualDamage;
}

bool AAbilityShooterCharacter::CanDie() const
{
	if (bIsDying || IsPendingKill() || Role != ROLE_Authority || !IsValid(GetWorld()->GetAuthGameMode()) || GetWorld()->GetAuthGameMode()->GetMatchState() == MatchState::LeavingMap)
		return false;

	return true;
}

bool AAbilityShooterCharacter::Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser)
{
	if (!CanDie())
		return false;

	health = FMath::Min(0.0f, health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (IsValid(GetController())) ? GetController() : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AAbilityShooterGameMode>()->ShooterKilled(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AAbilityShooterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, IsValid(Killer) ? Killer->GetPawn() : nullptr, DamageCauser);
	return true;
}

void AAbilityShooterCharacter::OnDeath(float KillingDamage, FDamageEvent const & DamageEvent, APawn * PawnInstigator, AActor * DamageCauser)
{
	if (bIsDying)
		return;

	bReplicateMovement = false;
	bTearOff = true;
	bIsDying = true;

	if (Role == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			/*UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, false, "Damage");
			}*/
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	//if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
		//UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());

	// remove all weapons
	//DestroyInventory();

	DetachFromControllerPendingDestroy();
	//StopAllAnimMontages();

	/*if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}*/



	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	/*float DeathAnimDuration = PlayAnimMontage(DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AShooterCharacter::SetRagdollPhysics, FMath::Min(0.1f, DeathAnimDuration), false);
	}
	else
	{
		SetRagdollPhysics();
	}*/

	SetRagdollPhysics();

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AAbilityShooterCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
		bInRagdoll = false;
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
		bInRagdoll = false;
	else
	{
		// initialize physics/etc
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
		SetLifeSpan(10.0f);
}

void AAbilityShooterCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	// jump, but only on the first touch
	if (FingerIndex == ETouchIndex::Touch1)
	{
		Jump();
	}
}

void AAbilityShooterCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		StopJumping();
	}
}

void AAbilityShooterCharacter::PrimaryUseStart()
{
}

void AAbilityShooterCharacter::PrimaryUseStop()
{
}

void AAbilityShooterCharacter::AlternateUseStart()
{
}

void AAbilityShooterCharacter::AlternateUseStop()
{
}

void AAbilityShooterCharacter::OnStartAbility(int32 abilityIndex)
{
}

void AAbilityShooterCharacter::OnStopAbility(int32 abilityIndex)
{
}

void AAbilityShooterCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());

	if (GetVelocity().SizeSquared() <= 0.f)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, 0.01f);
	}
}

void AAbilityShooterCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());

	if (GetVelocity().SizeSquared() <= 0.f)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, 0.01f);
	}
}

void AAbilityShooterCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AAbilityShooterCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

FName AAbilityShooterCharacter::GetEquipmentAttachPoint() const
{
	return equipmentAttachPoint;
}

bool AAbilityShooterCharacter::CanUseEquipment() const
{
	return IsAlive();
}

bool AAbilityShooterCharacter::IsAlive() const
{
	return health > 0.f;
}

bool AAbilityShooterCharacter::CanReload() const
{
	return true;
}

AEquipmentItem* AAbilityShooterCharacter::GetCurrentEquipment() const
{
	return currentEquipment;
}

void AAbilityShooterCharacter::OnRep_CurrentEquipment(AEquipmentItem* lastEquipment)
{
	SetCurrentEquipment(currentEquipment, lastEquipment);
}

void AAbilityShooterCharacter::SetCurrentEquipment(AEquipmentItem* newEquipment, AEquipmentItem* lastEquipment /* = nullptr */)
{
	AEquipmentItem* localLastEquipment = nullptr;

	if (IsValid(lastEquipment))
		localLastEquipment = lastEquipment;
	else if (newEquipment != currentEquipment)
		localLastEquipment = currentEquipment;

	if (IsValid(localLastEquipment))
		localLastEquipment->OnUnEquip();

	currentEquipment = newEquipment;

	if (IsValid(newEquipment))
	{
		newEquipment->SetOwningCharacter(this);
		newEquipment->OnEquip(lastEquipment);
	}
}

float AAbilityShooterCharacter::GetMaxHealth() const
{
	//@TODO: get the max HP from the Shooter's stats
	return 100.f;
}

//////////////////////////////////////////////////////////////////////////
// Replication

void AAbilityShooterCharacter::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE(AAbilityShooterCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout);
}

void AAbilityShooterCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, equipmentInventory, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	//DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, bIsTargeting, COND_SkipOwner);
	//DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, bWantsToRun, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(AAbilityShooterCharacter, currentEquipment);
	DOREPLIFETIME(AAbilityShooterCharacter, health);
}