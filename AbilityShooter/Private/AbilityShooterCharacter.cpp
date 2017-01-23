#include "AbilityShooter.h"
#include "AbilityShooterCharacter.h"
#include "AbilityShooterGameMode.h"
#include "PlayerHUD.h"
#include "EquipmentItem.h"
#include "AbilityShooterPlayerController.h"
#include "ASPlayerState.h"
#include "GameFramework/GameState.h"
#include "UnrealNetwork.h"
#include "BulletGunWeapon.h"
#include "DrawDebugHelpers.h"
#include "Ability.h"
#include "AbilityShooterMovementComponent.h"

//////////////////////////////////////////////////////////////////////////
// AAbilityShooterCharacter

AAbilityShooterCharacter::AAbilityShooterCharacter(const FObjectInitializer& objectInitializer)
:Super(objectInitializer.SetDefaultSubobjectClass<UAbilityShooterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Overlap);
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

	statsManager = CreateDefaultSubobject<UStatsManager>(TEXT("statsManager"));
	statsManager->SetOwningCharacter(this);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	
	health = 100.f;
	maxAbilityCount = 7;

	aboveHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("aboveHeadUI"));
	aboveHeadWidget->SetupAttachment(RootComponent, TEXT("hpbar"));
	aboveHeadWidget->SetWidgetClass(aboveHeadWidgetClass);
	aboveHeadWidget->SetDrawSize(FVector2D(0.f, 0.f));

	//create the headshot collision capsule
	headshotComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("headshotCapsule"));
	headshotComponent->SetupAttachment(GetMesh(), TEXT("headshot"));
	headshotComponent->ComponentTags.Add(TEXT("headshot"));
	headshotComponent->SetCapsuleHalfHeight(20.f);
	headshotComponent->SetCapsuleRadius(15.f);
	headshotComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	headshotComponent->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);

	NetCullDistanceSquared = 2500000000.f;
}

void AAbilityShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Role == ROLE_Authority)
	{
		health = GetMaxHealth();
		SpawnDefaultInventory();

		for (int32 i = 0; i < 7; i++)
			abilities.Add(nullptr);
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

	if (IsValid(aboveHeadWidget) && IsValid(aboveHeadWidget->GetUserWidgetObject()))
		aboveHeadWidget->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Hidden);
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

	//notify the player HUD that the current effects have changed
	APlayerController* pc = Cast<APlayerController>(GetController());
	if (IsValid(pc))
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (IsValid(hud))
			hud->OnEffectsListUpdate();
	}

	//@TODO: set mesh team color material instance
}

void AAbilityShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->MaxWalkSpeed = GetCurrentStat(EStat::ES_Move);
	}

	//get any usable objects
	if (IsLocallyControlled())
	{
		AActor* newUseObject = nullptr;
		const FVector start = GetFollowCamera()->GetComponentLocation();
		const FVector dir = GetFollowCamera()->GetComponentRotation().Vector();
		const float dist = (start - GetActorLocation()).Size();
		const FVector end = start + dir * (dist + 150.f);

		//DrawDebugLine(GetWorld(), start, end, FColor::Emerald, true, 5.f, 0, 0.5f);

		FHitResult hit;
		FCollisionQueryParams params;
		params.AddIgnoredActor(this);
		GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_WorldStatic, params);

		if (IsValid(hit.GetActor()) && hit.GetActor()->Tags.Contains(TEXT("usable")))
			newUseObject = hit.GetActor();

		const FVector end2 = start + dir * (dist + 5150.f);
		GetWorld()->LineTraceSingleByChannel(hit, start, end2, ECC_WorldStatic, params);

		if (IsValid(hit.GetActor()) && hit.GetActor()->IsA(AAbilityShooterCharacter::StaticClass()) && Cast<AAbilityShooterCharacter>(hit.GetActor())->IsAlive())
		{
			APlayerController* pc = Cast<APlayerController>(GetController());
			if (IsValid(pc))
			{
				APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
				if (IsValid(hud))
					hud->OnCharacterTargeted(Cast<AAbilityShooterCharacter>(hit.GetActor()));
			}
		}
		else
		{
			APlayerController* pc = Cast<APlayerController>(GetController());
			if (IsValid(pc))
			{
				APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
				if (IsValid(hud))
					hud->OnCharacterUnTargeted();
			}
		}

		currentUseObject = newUseObject;

		//check for close camera
		FVector camLoc = FollowCamera->GetComponentLocation();
		float dSquared = (camLoc - GetActorLocation()).SizeSquared();

		if (dSquared <= FMath::Square(115.f + GetCapsuleComponent()->GetScaledCapsuleRadius()) && IsValid(GetMesh()))
			GetMesh()->SetHiddenInGame(true, false);
		else if (IsValid(GetMesh()))
			GetMesh()->SetHiddenInGame(false, false);
	}
}

void AAbilityShooterCharacter::OnRep_ClientMoveSpeed()
{
	
}

void AAbilityShooterCharacter::AddAbility(TSubclassOf<AAbility> newType, bool bFromOutfit)
{
	//only run on server with a valid class
	if (Role < ROLE_Authority || !IsValid(newType))
		return;

	//abilities array is corrupt
	if (abilities.Num() != 7)
	{
		UE_LOG(LogTemp, Warning, TEXT("The abilities array for a Shooter is corrupt!"));
		return;
	}

	int selectedIndex = -1;

	AAbility* newAbDefault = Cast<AAbility>(newType->GetDefaultObject());

	if (newAbDefault->bUltimateAbility)
		selectedIndex = 2;
	else
	{
		if (bFromOutfit)
		{
			for (int32 i = 3; i <= 6; i++)
			{
				AAbility* test = abilities[i];
				if (!IsValid(test))
				{
					selectedIndex = i;
					break;
				}
			}
		}
		else
		{
			for (int32 i = 0; i <= 1; i++)
			{
				AAbility* test = abilities[i];
				if (!IsValid(test))
				{
					selectedIndex = i;
					break;
				}
			}
		}
	}

	//don't add if the Shooter already has an ultimate
	if (newAbDefault->bUltimateAbility)
	{
		for (AAbility* ability : abilities)
		{
			if (!IsValid(ability))
				continue;

			if (ability->bUltimateAbility)
				return;
		}
	}

	if (selectedIndex < 0 || selectedIndex >= 7)
		return;

	AAbility* newAbility = GetWorld()->SpawnActor<AAbility>(newType, GetActorLocation(), GetActorRotation());
	if (IsValid(newAbility))
	{
		newAbility->SetupAbility(this);
		abilities[selectedIndex] = newAbility;
	}
}

void AAbilityShooterCharacter::AddExistingAbility(AAbility* ability)
{
	//only run on server with a valid class
	if (Role < ROLE_Authority || !IsValid(ability))
		return;

	int32 selectedIndex = -1;
	for (int32 i = 0; i < abilities.Num(); i++)
	{
		if (!IsValid(abilities[i]))
		{
			selectedIndex = i;
			break;
		}
	}

	if (selectedIndex < 0 || selectedIndex >= 7)
		return;

	ability->SetupAbility(this);
	abilities[selectedIndex] = ability;
}

void AAbilityShooterCharacter::PossessedBy(class AController* C)
{
	Super::PossessedBy(C);

	//@TODO: set team color now that we have a player state on the server
}

AOutfit* AAbilityShooterCharacter::GetCurrentOutfit() const
{
	return currentOutfit;
}

void AAbilityShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//@TODO: set team color now that we have a player state on the client
}

bool AAbilityShooterCharacter::IsEnemyFor(AController* testPC) const
{
	if (testPC == GetController() || !IsValid(testPC))
		return false;

	AASPlayerState* testPlayerState = Cast<AASPlayerState>(testPC->PlayerState);
	AASPlayerState* thisPlayerState = Cast<AASPlayerState>(PlayerState);

	bool bIsEnemy = true;
	if (GetWorld()->GameState && GetWorld()->GameState->GameModeClass)
	{
		const AAbilityShooterGameMode* defGame = GetWorld()->GameState->GameModeClass->GetDefaultObject<AAbilityShooterGameMode>();
		if (IsValid(defGame) && IsValid(thisPlayerState) && IsValid(testPlayerState))
			bIsEnemy = defGame->CanDealDamage(testPlayerState, thisPlayerState);
	}

	return bIsEnemy;
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
		int32 ind = equipmentInventory.AddUnique(item);

		if (!IsValid(currentEquipment))
		{
			currentEquipmentIndex = ind;
			EquipEquipment(item);
		}
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

void AAbilityShooterCharacter::EquipOutfit(AOutfit* newOutfit, bool bReactivate)
{
	if (IsValid(newOutfit))
	{
		currentOutfit = newOutfit;
		currentOutfit->EquipOutfit(this, bReactivate);
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
	InputComponent->BindAction("Reload", IE_Pressed, this, &AAbilityShooterCharacter::OnTryReload);
	InputComponent->BindAction("Use", IE_Pressed, this, &AAbilityShooterCharacter::OnUseObjectStart);
	InputComponent->BindAction("Use", IE_Released, this, &AAbilityShooterCharacter::OnUseObjectStop);

	//change equipment
	InputComponent->BindAction("ProgressEquipWheelForward", IE_Pressed, this, &AAbilityShooterCharacter::OnProgressEquipWheelForward);
	InputComponent->BindAction("ProgressEquipWheelBack", IE_Pressed, this, &AAbilityShooterCharacter::OnProgressEquipWheelBackward);
	InputComponent->BindAction("CancelEquipWheel", IE_Pressed, this, &AAbilityShooterCharacter::EquipmentWheelTimedOut);

	//ability interaction
	InputComponent->BindAction("Ability1", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<0>);
	InputComponent->BindAction("Ability1", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<0>);
	InputComponent->BindAction("Ability2", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<1>);
	InputComponent->BindAction("Ability2", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<1>);
	InputComponent->BindAction("Ultimate", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<2>);
	InputComponent->BindAction("Ultimate", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<2>);
	InputComponent->BindAction("Ability4", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<3>);
	InputComponent->BindAction("Ability4", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<3>);
	InputComponent->BindAction("Ability5", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<4>);
	InputComponent->BindAction("Ability5", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<4>);
	InputComponent->BindAction("Ability6", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<5>);
	InputComponent->BindAction("Ability6", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<5>);
	InputComponent->BindAction("Ability7", IE_Pressed, this, &AAbilityShooterCharacter::UseAbilityStart<6>);
	InputComponent->BindAction("Ability7", IE_Released, this, &AAbilityShooterCharacter::UseAbilityStop<6>);
}

void AAbilityShooterCharacter::ReplicateHit(float Damage, struct FShooterDamage const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FShooterDamage const& LastDamageEvent = LastTakeHitInfo.GetShooterDamage();
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
	LastTakeHitInfo.SetShooterDamage(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AAbilityShooterCharacter::PlayHit(float DamageTaken, struct FShooterDamage const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
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

	//finally tell the UIs of both the damaging character and this character of this damage event
	AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(PawnInstigator->GetController());
	if (pc)
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (hud)
		{
			hud->OnCharacterDealtDamage(DamageEvent);
		}
	}
	pc = Cast<AAbilityShooterPlayerController>(GetController());
	if (pc)
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (hud)
		{
			hud->OnCharacterDamaged(DamageEvent);
		}
	}
}

void AAbilityShooterCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetShooterDamage(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	else
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetShooterDamage(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
}

float AAbilityShooterCharacter::TakeDamage(float Damage, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	//don't damage already dead characters
	if (health <= 0.f)
		return 0.0f;

	//@TODO: check for invulnerability

	//broadcast that this shooter was damaged
	FShooterDamage shooterDamage = *((FShooterDamage const*)(&DamageEvent));
	shooterDamage.PublicDamageType = shooterDamage.DamageTypeClass;
	if (IsValid(shooterDamage.EventInstigator) && IsValid(shooterDamage.EventInstigator->PlayerState))
		shooterDamage.DamageCauserInfo.damagingShooterName = shooterDamage.EventInstigator->PlayerState->PlayerName;
	if (IsValid(shooterDamage.DamageCauser))
	{
		AShooterItem* si = Cast<AShooterItem>(shooterDamage.DamageCauser);
		if (IsValid(si))
		{
			shooterDamage.DamageCauserInfo.damagingEntityName = si->uiName;
		}
	}

	/*if (OnShooterDamaged.IsBound())
		OnShooterDamaged.Execute(shooterDamage, Damage);*/

		//go through and let the OnShooterDamaged delegates modify the damage
	for (int32 i = 0; i < OnShooterDamagedEvents.Num(); i++)
	{
		if (OnShooterDamagedEvents[i].IsBound())
		{
			OnShooterDamagedEvents[i].Execute(shooterDamage, Damage, Damage);
		}
		else
			OnShooterDamagedEvents.RemoveAt(i);
	}

	//go through and let the OnShooterDealtDamaged delegates modify the damage
	AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(EventInstigator);
	AAbilityShooterCharacter* damagingShooter = nullptr;
	if (IsValid(pc))
	{
		damagingShooter = Cast<AAbilityShooterCharacter>(pc->GetCharacter());
		if (IsValid(damagingShooter))
		{
			for (int32 i = 0; i < damagingShooter->OnShooterDealtDamageEvents.Num(); i++)
			{
				if (damagingShooter->OnShooterDealtDamageEvents[i].IsBound())
				{
					damagingShooter->OnShooterDealtDamageEvents[i].Execute(shooterDamage, this, Damage, Damage);
				}
				else
					damagingShooter->OnShooterDealtDamageEvents.RemoveAt(i);
			}
		}
	}

	/*AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(EventInstigator);
	if (IsValid(pc))
	{
		AAbilityShooterCharacter* damagingShooter = Cast<AAbilityShooterCharacter>(pc->GetCharacter());
		if (IsValid(damagingShooter) && damagingShooter->IsAlive() && damagingShooter->OnShooterDealtDamage.IsBound())
			damagingShooter->OnShooterDealtDamage.Execute(shooterDamage, this, Damage);
	}*/

	//@TODO: let the gametype modify the damage
	//let the defensivestats then modify the damage
	float defStat = 0.f;
	if (DamageEvent.DamageTypeClass == UPhysicalDamage::StaticClass())
	{
		defStat = GetCurrentStat(EStat::ES_Def);
	}
	else if (DamageEvent.DamageTypeClass == USpecialDamage::StaticClass())
	{
		defStat = GetCurrentStat(EStat::ES_SpDef);
	}

	float defMultiplier;
	if (defStat >= 0.f)
	{
		defMultiplier = 100.f / (100.f + defStat);
	}
	else
	{
		defMultiplier = 1.5 - (100.f / (100.f - defStat));
	}
	Damage *= defMultiplier;

	//call the super version for blueprint hooks
	const float actualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	shooterDamage.PublicDamage = actualDamage;
	if (actualDamage > 0.f)
	{
		health -= actualDamage;
		if (health <= 0.f)
			Die(actualDamage, shooterDamage, EventInstigator, DamageCauser);
		else
			PlayHit(actualDamage, shooterDamage, IsValid(EventInstigator) ? EventInstigator->GetPawn() : nullptr, DamageCauser);

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

bool AAbilityShooterCharacter::CanPerformAbilities() const
{
	bool bIsAimingAnAbility = false;
	for (AAbility* ability : abilities)
	{
		if (!IsValid(ability))
			continue;

		if (ability->GetCurrentState() == EAbilityState::Aiming)
			bIsAimingAnAbility = true;
	}

	return !bIsAimingAnAbility && currentAilment.type != EAilment::AL_Knockup && currentAilment.type != EAilment::AL_Stun && GetWorldTimerManager().GetTimerRemaining(currentAction.timer) <= 0.f;
}

bool AAbilityShooterCharacter::Die(float KillingDamage, struct FShooterDamage const& DamageEvent, class AController* Killer, class AActor* DamageCauser)
{
	if (!CanDie())
		return false;

	health = FMath::Min(0.0f, health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (IsValid(GetController())) ? GetController() : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AAbilityShooterGameMode>()->ShooterKilled(Killer, KilledPlayer, this, DamageType, Cast<AShooterItem>(DamageCauser));

	NetUpdateFrequency = GetDefault<AAbilityShooterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, IsValid(Killer) ? Killer->GetPawn() : nullptr, DamageCauser);
	return true;
}

void AAbilityShooterCharacter::OnDeath(float KillingDamage, FShooterDamage const & DamageEvent, APawn * PawnInstigator, AActor * DamageCauser)
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

		//end all abilities that are being performed right now
		for (AAbility* ability : abilities)
		{
			if (!IsValid(ability))
				continue;

			if (ability->GetCurrentState() == EAbilityState::Performing)
				ability->ForceStopAbility();
		}
	}

	//end all effects currently effecting that don't persist through death, and save the ones that do
	AAbilityShooterPlayerController* pc = Cast<AAbilityShooterPlayerController>(GetController());
	for (int32 i = 0; i < currentEffects.Num(); i++)
	{
		UEffect* effect = currentEffects[i];
		if (!IsValid(effect))
			continue;

		if (IsValid(pc) && Role == ROLE_Authority)
		{
			//transfer things that need to be to the player controller
			//transfer persistent effects
			if (effect->bPersistThruDeath)
			{
				FEffectInitInfo persistentInfo;
				persistentInfo.bDoesPersistThruDeath = effect->bPersistThruDeath;
				persistentInfo.description = effect->description;
				persistentInfo.duration = GetWorldTimerManager().GetTimerRemaining(effect->expirationTimer) > 0.f ? GetWorldTimerManager().GetTimerRemaining(effect->expirationTimer) : 0.f;
				persistentInfo.persistentTime = GetWorld()->TimeSeconds;
				persistentInfo.effectType = effect->GetClass();
				persistentInfo.statAlters = effect->statAlters;
				persistentInfo.uiName = effect->uiName;
				persistentInfo.persistentTimer = effect->expirationTimer;
				persistentInfo.persistentKey = effect->key;

				pc->persistentEffects.Add(persistentInfo);
			}
		}

		EndEffect(effect);

		i--;
	}

	//end all ailments
	EndCurrentAilment();
	ailmentQueue.Empty();

	//remove weapons from inventory
	if (IsValid(currentEquipment))
		currentEquipment->OnLeaveInventory();

	if (IsValid(pc) && Role == ROLE_Authority)
	{
		//transfer abilities
		for (int32 i = 0; i < abilities.Num(); i++)
		{
			if (i > 2)
				break;

			if (IsValid(abilities[i]))
			{
				abilities[i]->SetupAbility(nullptr);
				pc->persistentAbilities.AddUnique(abilities[i]);
			}
			
		}

		abilities.Empty();

		//transfer the outfit
		pc->persistentOutfit = currentOutfit;
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

	//finally tell the UIs of both the damaging character and this character of this damage event
	pc = Cast<AAbilityShooterPlayerController>(PawnInstigator->GetController());
	if (pc)
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (hud)
		{
			hud->OnCharacterDealtDamage(DamageEvent);
		}
	}
	pc = Cast<AAbilityShooterPlayerController>(GetController());
	if (pc)
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (hud)
		{
			hud->OnCharacterDamaged(DamageEvent);
		}
	}

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
	//check for confirming aim in abilities first
	for (AAbility* ability : abilities)
	{
		if (!IsValid(ability))
			continue;

		if (ability->GetCurrentState() == EAbilityState::Aiming)
		{
			ability->ConfirmAim();
			return;
		}
	}

	//then check to see if we need to confirm the equip wheel
	if (bIsEquipmentWheelActive)
	{
		ConfirmEquipmentWheelSelection();
		return;
	}

	if (!bWantsToUse)
	{
		bWantsToUse = true;
		if (IsValid(currentEquipment))
			currentEquipment->StartUse();
	}
}

void AAbilityShooterCharacter::PrimaryUseStop()
{
	if (bWantsToUse)
	{
		bWantsToUse = false;
		if (IsValid(currentEquipment))
			currentEquipment->StopUse();
	}
}

void AAbilityShooterCharacter::AlternateUseStart()
{
	if (!bWantsToUseAlt)
	{
		bWantsToUseAlt = true;
		if (IsValid(currentEquipment))
			currentEquipment->StartAlt();
	}
}

void AAbilityShooterCharacter::AlternateUseStop()
{
	if (bWantsToUseAlt)
	{
		bWantsToUseAlt = false;
		if (IsValid(currentEquipment))
			currentEquipment->StopAlt();
	}
}

void AAbilityShooterCharacter::OnStartAbility(int32 abilityIndex)
{
	if (abilityIndex >= 0 && abilityIndex < abilities.Num() && IsValid(abilities[abilityIndex]))
		abilities[abilityIndex]->StartPerform();
}

void AAbilityShooterCharacter::OnStopAbility(int32 abilityIndex)
{
	if (abilityIndex >= 0 && abilityIndex < abilities.Num() && IsValid(abilities[abilityIndex]))
		abilities[abilityIndex]->StopPerform(true);
}

void AAbilityShooterCharacter::OnProgressEquipWheelForward()
{
	StartEquipmentChangeWheel();
}

void AAbilityShooterCharacter::OnProgressEquipWheelBackward()
{
	StartEquipmentChangeWheel(false);
}

void AAbilityShooterCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AAbilityShooterCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AAbilityShooterCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && currentAilment.type != EAilment::AL_Stun)
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
	if ( (Controller != NULL) && (Value != 0.0f) && currentAilment.type != EAilment::AL_Stun)
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
		SendInterruptToAbilities(EAbilityInterruptSignal::VelocityDirectionChanged);
	}
}

FName AAbilityShooterCharacter::GetEquipmentAttachPoint() const
{
	return equipmentAttachPoint;
}

void AAbilityShooterCharacter::StartEquipmentChangeWheel(bool bShouldProgressForward /* = true */)
{
	if (IsAlive())
	{
		bIsEquipmentWheelActive = true;
		int32 progAmt = 1 * bShouldProgressForward ? 1 : -1;
		lastSelectedEquipmentIndex = currentEquipmentIndex;
		currentEquipmentIndex = FMath::Max(0, FMath::Min(equipmentInventory.Num() - 1, currentEquipmentIndex + progAmt));

		APlayerController* pc = Cast<APlayerController>(GetController());
		if (pc)
		{
			APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
			if (hud)
				hud->OnShowEquipmentWheel();
		}

		GetWorldTimerManager().SetTimer(equipWheelTimer, this, &AAbilityShooterCharacter::EquipmentWheelTimedOut, equipWheelTimeout);
	}
}

void AAbilityShooterCharacter::EquipmentWheelTimedOut()
{
	if (IsAlive())
	{
		bIsEquipmentWheelActive = false;

		currentEquipmentIndex = lastSelectedEquipmentIndex;

		APlayerController* pc = Cast<APlayerController>(GetController());
		if (pc)
		{
			APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
			if (hud)
				hud->OnHideEquipmentWheel();
		}
	}
}

void AAbilityShooterCharacter::ConfirmEquipmentWheelSelection()
{
	bIsEquipmentWheelActive = false;

	GetWorldTimerManager().ClearTimer(equipWheelTimer);

	if (currentEquipmentIndex >= 0 && currentEquipmentIndex < equipmentInventory.Num())
	{
		AEquipmentItem* equip = equipmentInventory[currentEquipmentIndex];
		EquipEquipment(equip);
	}

	APlayerController* pc = Cast<APlayerController>(GetController());
	if (pc)
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (hud)
			hud->OnHideEquipmentWheel();
	}
}

bool AAbilityShooterCharacter::CanUseEquipment() const
{
	return IsAlive() && GetWorldTimerManager().GetTimerRemaining(currentAction.timer) <= 0.f && currentAilment.type != EAilment::AL_Stun;
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

void AAbilityShooterCharacter::OnRep_CurrentOutfit()
{
	if (IsValid(currentOutfit))
		currentOutfit->EquipOutfit(this);
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
	return GetCurrentStat(EStat::ES_HP);
}

float AAbilityShooterCharacter::GetADSWeaponSpread() const
{
	return 0.4f;
}

void AAbilityShooterCharacter::ApplyEffect_Implementation(AAbilityShooterCharacter* originChar, const FEffectInitInfo& initInfo)
{
	//generate key
	FString newKey = initInfo.persistentKey;
	if (newKey == "")
	{
		if(IsValid(originChar))
			newKey += originChar->GetName() + "_";
		newKey += initInfo.uiName.ToString() + "_" + GetName();
	}

	//don't apply more than once
	for (UEffect* effect : currentEffects)
	{
		if (effect->GetKey() == newKey)
			return;
	}

	//create and initialize the effect
	UEffect* newEffect;
	if (!IsValid(initInfo.effectType))
		newEffect = NewObject<UEffect>();
	else
		newEffect = NewObject<UEffect>(this, initInfo.effectType);

	if (!IsValid(newEffect))
		return;

	newEffect->key = newKey;
	newEffect->Initialize(initInfo, this);

	//start this expiration timer with the needed time and add it to the list of effects
	newEffect->SetExpirationTimer();
	currentEffects.AddUnique(newEffect);

	//notify the player HUD that the current effects have changed
	APlayerController* pc = Cast<APlayerController>(GetController());
	if (IsValid(pc))
	{
		APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
		if (IsValid(hud))
			hud->OnEffectsListUpdate();
	}

	//if we're not the server, just set the expiration timer and add
	if (Role < ROLE_Authority)
		return;

	//blueprint on application event
	newEffect->OnEffectAppliedToCharacter(this);
}

void AAbilityShooterCharacter::EndEffect(UEffect* endingEffect)
{
	if (!IsValid(endingEffect))
		return;

	//clear the effects timer
	GetWorldTimerManager().ClearTimer(endingEffect->expirationTimer);

	//let the effect do any blueprint operations
	if (Role == ROLE_Authority)
		endingEffect->OnEffectRemovedFromCharacter(this);

	//remove the effect from our arrays
	currentEffects.Remove(endingEffect);

	//notify the player HUD that the current effects have changed
	if (IsLocallyControlled())
	{
		APlayerController* pc = Cast<APlayerController>(GetController());
		if (IsValid(pc))
		{
			APlayerHUD* hud = Cast<APlayerHUD>(pc->GetHUD());
			if (IsValid(hud))
				hud->OnEffectsListUpdate();
		}
	}
}

void AAbilityShooterCharacter::EndEffectByKey(const FString& key)
{
	for (int32 i = 0; i < currentEffects.Num(); i++)
	{
		if (currentEffects[i]->GetKey() == key)
		{
			EndEffect(currentEffects[i]);
			break;
		}
	}
}

void AAbilityShooterCharacter::ForceEndEffect_Implementation(const FString& key)
{
	for (UEffect* effect : currentEffects)
	{
		if (effect->key == key)
		{
			EndEffect(effect);
			break;
		}
	}
}

float AAbilityShooterCharacter::GetCurrentStat(EStat stat) const
{
	float statDelta = IsValid(statsManager) ? statsManager->GetCurrentStatValue(stat) : 0.f;

	//next, go thru the effects for the character and factor in those stat changes
	for (UEffect* effect : currentEffects)
	{
		for (FEffectStatAlter statAlter : effect->statAlters)
		{
			if (statAlter.alteredStat == stat)
				statDelta += statAlter.deltaStat;
		}
	}

	//next let outfits change the stats
	if (IsValid(currentOutfit) && IsValid(statsManager))
	{
		FBaseStats outfitStats = currentOutfit->GetDeltaStats();
		statDelta = statsManager->GetStatFromBaseStatAddition(statDelta, stat, outfitStats);
	}

	switch (stat)
	{
	case EStat::ES_EquipUseRate:
		if (IsValid(currentEquipment) && statDelta > 0.f)
			return currentEquipment->timesBetweenUse / statDelta;
		else if (IsValid(currentEquipment))
			return currentEquipment->timesBetweenUse;
	case EStat::ES_CritRatio:
		return statDelta > 0.f ? baseStats.critRatio * statDelta : baseStats.critRatio;
	case EStat::ES_Acc:
		return statDelta > 0.f ? baseStats.accuracy * statDelta : baseStats.accuracy;
	}

	return statDelta;
}

/*void AAbilityShooterCharacter::OnRep_Ailment()
{
	APlayerController* pc = Cast<APlayerController>(GetController());

	switch (currentAilment.type)
	{
	case EAilment::AL_None:
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (IsValid(pc))
			pc->SetIgnoreMoveInput(false);
		break;
	case EAilment::AL_Knockup: //a knockup is a stun that displaces the character a certain distance.
		GetCharacterMovement()->Velocity += currentAilment.dir;
		if (IsValid(pc))
			pc->SetIgnoreMoveInput(true);
		break;
	case EAilment::AL_Stun:
		GetCharacterMovement()->DisableMovement();
		break;
	}

	if (currentAilment.duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(ailmentTimer, currentAilment.duration, false);
	}
	else
		GetWorldTimerManager().ClearTimer(ailmentTimer);
}*/

void AAbilityShooterCharacter::ApplyAilment_Implementation(const FAilmentInfo& info)
{
	if (!IsAlive())
		return;

	if (currentAilment.type != EAilment::AL_None)
	{
		ailmentQueue.Enqueue(info);
		return;
	}

	currentAilment = info;
	APlayerController* pc = Cast<APlayerController>(GetController());

	switch (currentAilment.type)
	{
	case EAilment::AL_None:
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (IsValid(pc))
			pc->SetIgnoreMoveInput(false);
		break;
	case EAilment::AL_Knockup: //a knockup is a stun that displaces the character a certain distance.
		GetCharacterMovement()->Launch(info.dir);
		if (IsValid(pc))
			pc->SetIgnoreMoveInput(true);
		break;
	case EAilment::AL_Stun:
		if (IsValid(pc))
			pc->SetIgnoreMoveInput(true);
		break;
	}

	if (currentAilment.duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(ailmentTimer, this, &AAbilityShooterCharacter::EndCurrentAilment, currentAilment.duration);

		//attach an emitter for this ailment
		currentAilment.psComponent = UGameplayStatics::SpawnEmitterAttached(info.fx, GetRootComponent(), NAME_None, FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 25.f), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
		if (IsValid(currentAilment.psComponent))
		{
			currentAilment.psComponent->SetFloatParameter(TEXT("lifetime"), info.duration);
			currentAilment.psComponent->Activate(true);
		}
	}

	SendInterruptToAbilities(EAbilityInterruptSignal::AilmentAcquired);
}

FAilmentInfo AAbilityShooterCharacter::GetCurrentAilment() const
{
	return currentAilment;
}

void AAbilityShooterCharacter::EndCurrentAilment()
{
	//clear particle effects if ending early
	if (GetWorldTimerManager().GetTimerRemaining(ailmentTimer) > 0.f && IsValid(currentAilment.psComponent))
	{
		currentAilment.psComponent->Deactivate();
		currentAilment.psComponent = nullptr;
	}
	GetWorldTimerManager().ClearTimer(ailmentTimer);

	FAilmentInfo nextAilment;
	currentAilment = nextAilment;

	if (!ailmentQueue.IsEmpty())
	{
		ailmentQueue.Dequeue(nextAilment);
	}

	ApplyAilment(nextAilment);
}

bool AAbilityShooterCharacter::ShouldQuickAimAbilities() const
{
	//@DEBUG: testing right now, eventually read a setting from the player
	return true;
}

void AAbilityShooterCharacter::ApplyLatentAction(FCharacterActionInfo actionInfo, FLatentActionInfo latentInfo)
{
	if (GetWorld() && GetWorldTimerManager().GetTimerRemaining(currentAction.timer) <= 0.f)
	{
		FLatentActionManager& actionManager = GetWorld()->GetLatentActionManager();
		if (actionManager.FindExistingAction<FCharacterAction>(latentInfo.CallbackTarget, latentInfo.UUID) == nullptr)
		{
			FCharacterAction* newAction = new FCharacterAction(actionInfo.duration, latentInfo);
			actionManager.AddNewAction(latentInfo.CallbackTarget, latentInfo.UUID, newAction);
			actionInfo.latentAction = newAction;
			AllApplyAction(actionInfo);
		}
	}
}

void AAbilityShooterCharacter::AllApplyAction_Implementation(const FCharacterActionInfo& newAction)
{
	currentAction = newAction;

	GetWorldTimerManager().SetTimer(currentAction.timer, currentAction.duration, false);
}

void AAbilityShooterCharacter::ForceEndCurrentAction_Implementation()
{
	if (GetWorldTimerManager().GetTimerRemaining(currentAction.timer) > 0.f)
	{
		GetWorldTimerManager().ClearTimer(currentAction.timer);
		if (currentAction.latentAction != nullptr)
			currentAction.latentAction->bShouldEndPrematurely = true;

		currentAction = FCharacterActionInfo();
	}
}

AASPlayerState* AAbilityShooterCharacter::GetASPlayerState() const
{
	return Cast<AASPlayerState>(PlayerState);
}

int32 AAbilityShooterCharacter::GetMaxAbilityCount() const
{
	return maxAbilityCount;
}

void AAbilityShooterCharacter::OnTryReload()
{
	ABulletGunWeapon* weap = Cast<ABulletGunWeapon>(currentEquipment);
	if (IsValid(weap))
	{
		weap->StartReload();
	}
}

void AAbilityShooterCharacter::OnUseObjectStart()
{
	ServerUseCurrentObjectStarted(currentUseObject);
}

void AAbilityShooterCharacter::OnUseObjectStop()
{
	ServerUseCurrentObjectStopped(currentUseObject);
}

bool AAbilityShooterCharacter::ServerUseCurrentObjectStarted_Validate(AActor* useObject)
{
	return true;
}

void AAbilityShooterCharacter::ServerUseCurrentObjectStarted_Implementation(AActor* useObject)
{
	if (IsValid(useObject))
	{
		//try equipping it if we're 'using' (picking up) equipment
		AEquipmentItem* equip = Cast<AEquipmentItem>(useObject);
		if (IsValid(equip))
		{
			AddEquipment(equip);
			EquipEquipment(equip);
		}
	}
}

bool AAbilityShooterCharacter::ServerUseCurrentObjectStopped_Validate(AActor* useObject)
{
	return true;
}

void AAbilityShooterCharacter::ServerUseCurrentObjectStopped_Implementation(AActor* useObject)
{
	
}

void AAbilityShooterCharacter::SendInterruptToAbilities(EAbilityInterruptSignal signal)
{
	for (AAbility* ability : abilities)
	{
		if (!IsValid(ability))
			continue;

		if (ability->GetCurrentState() == EAbilityState::Performing)
			ability->HandleInterrupt(signal);
	}
}

int32 AAbilityShooterCharacter::GetEffectStacks(const FString& key) const
{
	for (UEffect* effect : currentEffects)
	{
		if (IsValid(effect) && effect->GetKey() == key)
			return effect->GetStacks();
	}

	return -1;
}

void AAbilityShooterCharacter::AddEffectStacks_Implementation(const FString& key, int32 deltaAmt, bool bShouldResetEffectTimer)
{
	for (UEffect* effect : currentEffects)
	{
		if (IsValid(effect) && effect->GetKey() == key)
		{
			effect->AddStacks(deltaAmt);
			if (bShouldResetEffectTimer)
				effect->SetExpirationTimer();
		}
	}
}

void AAbilityShooterCharacter::SetEffectStacks_Implementation(const FString& key, int32 newAmt, bool bShouldResetEffectTimer)
{
	for (UEffect* effect : currentEffects)
	{
		if (IsValid(effect) && effect->GetKey() == key)
		{
			effect->SetStacks(newAmt);
			if (bShouldResetEffectTimer)
				effect->SetExpirationTimer();
		}
	}
}

void AAbilityShooterCharacter::UpgradeOutfit_Implementation(uint8 tree, uint8 row, uint8 col)
{
	if (IsValid(currentOutfit))
		currentOutfit->Upgrade(tree, row, col);
}

FRotator AAbilityShooterCharacter::GetAbilityControlRotation() const
{
	//return the replicated one first if available
	AASPlayerState* ps = Cast<AASPlayerState>(PlayerState);
	if (IsValid(ps))
		return ps->viewRotation;

	return GetControlRotation();
}

void AAbilityShooterCharacter::StartDash(const FVector& dashEndLocation, float spdScale)
{
	UAbilityShooterMovementComponent* amc = Cast<UAbilityShooterMovementComponent>(GetCharacterMovement());
	if (IsValid(amc))
	{
		amc->DashLaunch(dashEndLocation, spdScale);
		OnShooterDashStarted.Broadcast(dashEndLocation, spdScale);
	}
}

void AAbilityShooterCharacter::StopDash()
{
	UAbilityShooterMovementComponent* amc = Cast<UAbilityShooterMovementComponent>(GetCharacterMovement());
	if (IsValid(amc))
	{
		amc->EndDash();
	}
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
	//DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, abilities, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	//DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, bIsTargeting, COND_SkipOwner);
	//DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, bWantsToRun, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AAbilityShooterCharacter, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(AAbilityShooterCharacter, currentEquipment);
	DOREPLIFETIME(AAbilityShooterCharacter, health);
	//DOREPLIFETIME(AAbilityShooterCharacter, currentAilment); 
	DOREPLIFETIME(AAbilityShooterCharacter, abilities);
	//DOREPLIFETIME(AAbilityShooterCharacter, clientMoveSpeed);
	DOREPLIFETIME(AAbilityShooterCharacter, currentOutfit);
}