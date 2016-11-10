#pragma once
#include "GameFramework/Character.h"
#include "AbilityShooterTypes.h"
#include "Sound/SoundCue.h"
#include "Effect.h"
#include "CharacterAction.h"
#include "Outfit.h"
#include "ShooterDamage.h"

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"

#include "AbilityShooterCharacter.generated.h"

class AEquipmentItem;
class AAbility;
class AASPlayerState;

/* Shooter damage event declaration */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShooterDamagedDelegate, FShooterDamage, damageInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShooterDealtDamageDelegate, FShooterDamage, damageInfo, AAbilityShooterCharacter*, damagedCharacter);

/* types for hard Crowd Control (Ailments) */
UENUM(BlueprintType)
enum class EAilment : uint8
{
	AL_None UMETA(DisplayName = "No Ailment"),
	AL_Knockup UMETA(DisplayName = "Knocked Up"),
	AL_Stun UMETA(DisplayName = "Stunned"),
	AL_Neutral UMETA(DisplayName = "Neutralized"),
	AL_Blind UMETA(DisplayName = "Blinded"),
	AL_Snare UMETA(DisplayName = "Immobilzed"),
	AL_Max UMETA(Hidden)
};

/* struct for holding all of the data that an ailment needs */
USTRUCT(BlueprintType)
struct FAilmentInfo
{
	GENERATED_USTRUCT_BODY()

	/* type of ailment this is */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ailment)
	EAilment type;

	/* text to represent the ailment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ailment)
	FText text;

	/* duration this ailment normally lasts for */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ailment)
	float duration;

	/* any directional and magnitude info associated with the ailment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ailment)
	FVector dir;

	/* HUD icon associated with this CC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ailment)
	UTexture* icon;

	/* particle system associated with this CC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ailment)
	UParticleSystem* fx;

	FAilmentInfo()
	{
		type = EAilment::AL_None;
		text = FText::GetEmpty();
		duration = -1.f;
		icon = nullptr;
		fx = nullptr;
	}
};

/* struct for holding all of the data that a character action needs */
USTRUCT(BlueprintType)
struct FCharacterActionInfo
{
	GENERATED_USTRUCT_BODY()

	/* text to represent the action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterAction)
	FText title;

	/* how long the action lasts overall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterAction)
	float duration;

	/* timer to track the actions time left */
	UPROPERTY(BlueprintReadWrite, Category = CharacterAction)
	FTimerHandle timer;

	/* latent action we can use to prematurely return on if we need to */
	FCharacterAction* latentAction;

	FCharacterActionInfo()
	{
		title = FText::GetEmpty();
		duration = -1.f;
		latentAction = nullptr;
	}
};

UCLASS(config=Game)
class AAbilityShooterCharacter : public ACharacter
{
	friend class UStatsManager;
	friend class AAbility;
	friend class AAbilityShooterPlayerController;
	friend class AAbilityShooterGameMode;

	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	/* widget component for above head character info */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* aboveHeadWidget;

	/* template widget class for the aboveHeadWidget */
	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSubclassOf<UUserWidget> aboveHeadWidgetClass;

public:
	AAbilityShooterCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/* --EVENT DELEGATES --------------------------------------------------------*/

	/* delegate to handle shooter damage events */
	UPROPERTY(BlueprintAssignable)
	FShooterDamagedDelegate OnShooterDamaged;

	/* delegate to handle when this shooter deals damage */
	UPROPERTY(BlueprintAssignable)
	FShooterDealtDamageDelegate OnShooterDealtDamage;

protected:

	/* current usable object within our reach */
	UPROPERTY(BlueprintReadOnly, Category = UseObject)
	AActor* currentUseObject;

	/** default inventory list */
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	TArray<TSubclassOf<AEquipmentItem> > defaultEquipmentList;

	/** current equipment in inventory */
	UPROPERTY(Transient, Replicated)
	TArray<AEquipmentItem*> equipmentInventory;

	/** currently equipped equipment */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentEquipment)
	AEquipmentItem* currentEquipment;

	/** current abilities in inventory */
	UPROPERTY(Transient, Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	TArray<AAbility*> abilities;

	/* max number of abilities this Shooter can have */
	int32 maxAbilityCount;

	/* current outfit this shooter has equipped */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentOutfit)
	AOutfit* currentOutfit;

	/** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	float LastTakeHitTimeTimeout;

	/** Replicate where this pawn was last hit and damaged */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_LastTakeHitInfo)
	struct FTakeHitInfo LastTakeHitInfo;

	/** socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	FName equipmentAttachPoint;

	/** effect played on respawn */
	UPROPERTY(EditDefaultsOnly, Category = Character)
	UParticleSystem* respawnFX;

	/** sound played on respawn */
	UPROPERTY(EditDefaultsOnly, Category = Character)
	USoundCue* respawnSound;

	/* array of effects currently afflicting this character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Effects)
	TArray<UEffect*> currentEffects;

	/* current ailment (if any) affecting this Shooter */
	UPROPERTY()
	FAilmentInfo currentAilment;

	/* array of ailments that need to be inflicted to the character (if there is an ailment currently being processed) */
	TQueue<FAilmentInfo> ailmentQueue;

	/* timer for handling aimlments */
	UPROPERTY(BlueprintReadOnly, Category = Ailment)
	FTimerHandle ailmentTimer;

	/* current action (if any) this character is performing */
	UPROPERTY(BlueprintReadOnly, Category = CharacterAction)
	FCharacterActionInfo currentAction;

	/* base stats for this character type */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Stats)
	FBaseStats baseStats;

	/* client movespeed that is updated by the server */
	UPROPERTY()
	float clientMoveSpeed;

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	/** Update the character. (Running, health etc). */
	virtual void Tick(float DeltaSeconds) override;

	/** cleanup inventory */
	virtual void Destroyed() override;

	/** [server] spawns default inventory */
	void SpawnDefaultInventory();

	/** [server] remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/** update mesh for first person view */
	virtual void PawnClientRestart() override;

	/** [server] perform PlayerState related setup */
	virtual void PossessedBy(class AController* C) override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;

	/* adds an equipment item to the inventory */
	void AddEquipment(AEquipmentItem* item);

	/* removes an equipment item from the inventory */
	void RemoveEquipment(AEquipmentItem* item);

	/* finds the first instance of a matching class of equipment from inventory */
	AEquipmentItem* FindEquipment(TSubclassOf<AEquipmentItem> equipmentClass);

	/* equips a weapon from inventory for both the client and server */
	void EquipEquipment(AEquipmentItem* item);

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface

	/** sets up the replication for taking a hit */
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	/** play effects on hit */
	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser);

	/** notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser);

	/** switch to ragdoll */
	void SetRagdollPhysics();

	/** play hit or death on client */
	UFUNCTION()
	void OnRep_LastTakeHitInfo();

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentEquipment(AEquipmentItem* lastEquipment);

	/* whenever the outfit is replicated to clients */
	UFUNCTION()
	void OnRep_CurrentOutfit();

	/* notify the client of Ailment */
	//UFUNCTION()
	//void OnRep_Ailment();

	UFUNCTION()
	void OnRep_ClientMoveSpeed();

	/** Called on the actor right before replication occurs */
	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	/** equip weapon */
	UFUNCTION(reliable, server, WithValidation)
	void ServerEquipEquipment(AEquipmentItem* newEquipment);

	/* server use object */
	UFUNCTION(reliable, server, WithValidation)
	void ServerUseCurrentObjectStarted(AActor* useObject);

	/* server use object */
	UFUNCTION(reliable, server, WithValidation)
	void ServerUseCurrentObjectStopped(AActor* useObject);

public:

	/* current stats manager for this character (can be shared with the owning player controller) */
	UPROPERTY(BlueprintReadOnly, Category = Stats)
	UStatsManager* statsManager;

	// Current health of the Shooter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Health)
	float health;

	/** Identifies if the Shooter is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
	uint32 bIsDying : 1;

	/* does this character want to use their equipment? */
	UPROPERTY(BlueprintReadOnly, Category = Use)
	bool bWantsToUse = false;

	/* does this character want to use their equipment's alt? */
	UPROPERTY(BlueprintReadOnly, Category = Use)
	bool bWantsToUseAlt = false;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** handler for primary equipment use starting*/
	void PrimaryUseStart();

	/** handler for primary equipment use stopping */
	void PrimaryUseStop();

	/** handler for alternate equipment use starting */
	void AlternateUseStart();

	/** handler for alternate equipment use stopping */
	void AlternateUseStop();

	/** template version for simple input */
	template<int Index>
	void UseAbilityStart()
	{
		OnStartAbility(Index);
	}

	/** template version for simple input finished */
	template<int Index>
	void UseAbilityStop()
	{
		OnStopAbility(Index);
	}

	/* handler for trying to reload the current equipment */
	void OnTryReload();

	/* handler for using nearby aimed at objects started */
	void OnUseObjectStart();

	/* handler for using nearby aimed at objects stopped */
	void OnUseObjectStop();

	/* override take damage function to allow for damage to drain HP and be modified */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Returns True if the Shooter can die in the current state */
	virtual bool CanDie() const;

	/**
	* Kills Shooter.  Server only.
	* @param KillingDamage - Damage amount of the killing blow
	* @param DamageEvent - Damage event of the killing blow
	* @param Killer - Who killed this Shooter
	* @param DamageCauser - the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
	* @returns true if allowed
	*/
	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	/* gets the attach point for equipment */
	UFUNCTION(BlueprintCallable, Category = Equipment)
	FName GetEquipmentAttachPoint() const;

	/* whether or not this character can use equipment */
	bool CanUseEquipment() const;

	/* whether or not the character is alive */
	UFUNCTION(BlueprintCallable, Category = Health)
	bool IsAlive() const;

	/** check if character can reload weapon */
	bool CanReload() const;

	/** get currently equipped equipment */
	UFUNCTION(BlueprintCallable, Category = "Game|Equipment")
	AEquipmentItem* GetCurrentEquipment() const;

	/** get currently equipped outfit */
	UFUNCTION(BlueprintCallable, Category = "Game|Outfit")
	AOutfit* GetCurrentOutfit() const;

	/** updates current weapon */
	void SetCurrentEquipment(AEquipmentItem* newEquipment, AEquipmentItem* lastEquipment = nullptr);

	/** get max health */
	UFUNCTION(BlueprintCallable, Category=Health)
	float GetMaxHealth() const;

	/** get aim offsets */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	FRotator GetAimOffsets() const;

	/* gets the percent of reduction in max weapon spread while this character is aiming down sights */
	float GetADSWeaponSpread() const;

	/* applies an effect to this character */
	UFUNCTION(BlueprintCallable, reliable, NetMulticast, Category = Effect)
	void ApplyEffect(AAbilityShooterCharacter* originChar, const FEffectInitInfo& initInfo);

	/* [SERVER] ends and removes an effect that's currently affecting this character */
	UFUNCTION(BlueprintCallable, Category = Effect)
	void EndEffect(UEffect* endingEffect);

	/* [SERVER] ends and removes an effect that's currently affecting this character by key */
	UFUNCTION(BlueprintCallable, Category = Effect)
	void EndEffectByKey(UPARAM(ref) const FString& key);

	/* force end an effect (useful for ending effects that aren't expired yet) */
	UFUNCTION(BlueprintCallable, Category=Effect, reliable, NetMulticast)
	void ForceEndEffect(const FString& key);

	/* gets the current value (modified by effects) for a stat */
	UFUNCTION(BlueprintCallable, Category = Effect)
	float GetCurrentStat(EStat stat) const;

	/* called whenever something tries to give this character an Ailment */
	UFUNCTION(BlueprintCallable, reliable, NetMulticast, Category = CC)
	void ApplyAilment(const FAilmentInfo& info);

	/* called whenever something tries to end this character's current Ailment */
	UFUNCTION(BlueprintCallable, Category = CC)
	void EndCurrentAilment();

	/* called to get the current Ailment status of this character */
	UFUNCTION(BlueprintCallable, Category = CC)
	FAilmentInfo GetCurrentAilment() const;

	/* whether or not this Shooter can perform Abilities */
	bool CanPerformAbilities() const;

	/* [server] adds a type of ability to this character's inventory */
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void AddAbility(TSubclassOf<AAbility> newType, bool bFromOutfit = false);

	/* [server] adds an already exisiting ability to this character's inventory */
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void AddExistingAbility(AAbility* ability);

	/* whether or not this character is an enemy for a controller */
	UFUNCTION(BlueprintCallable, Category = Enemy)
	bool IsEnemyFor(AController* testPC) const;

	/* whether or not this character needs to enter an Ability's 'Aiming' state or can go directly to performing (gets if the player controller has bUseQuickAimForAbilities) */
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool ShouldQuickAimAbilities() const;

	/* latent blueprint function that sets the current character action and doesn't return until the action is */
	UFUNCTION(BlueprintCallable, Category = CharacterAction, meta = (Latent, LatentInfo = "latentInfo"))
	void ApplyLatentAction(FCharacterActionInfo actionInfo, FLatentActionInfo latentInfo);

	/* net multicast function to broadcast to all clients about an action */
	UFUNCTION(reliable, NetMulticast, BlueprintCallable, Category=Action)
	void AllApplyAction(const FCharacterActionInfo& newAction);

	/* forcibly end the current action */
	UFUNCTION(reliable, NetMulticast, BlueprintCallable, Category = Action)
	void ForceEndCurrentAction();

	/* get casted player state */
	AASPlayerState* GetASPlayerState() const;

	/* get the max number of abilities this character can have */
	UFUNCTION(BlueprintCallable, Category = Abilities)
	int32 GetMaxAbilityCount() const;

	/** handler for starting abilities */
	void OnStartAbility(int32 abilityIndex);

	/** handler for stopping abilities */
	void OnStopAbility(int32 abilityIndex);

	/* send an interrupt signal to all abilities that are being performed. let the ability decide what to do with the signal */
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SendInterruptToAbilities(EAbilityInterruptSignal signal);

	/* get the stacks of an effect */
	UFUNCTION(BlueprintCallable, Category = Abilities)
	int32 GetEffectStacks(UPARAM(ref) const FString& key) const;

	/* (must be called on server) add to stacks of an effect */
	UFUNCTION(BlueprintCallable, reliable, NetMulticast, Category = Abilities)
	void AddEffectStacks(const FString& key, int32 deltaAmt, bool bShouldResetEffectTimer = true);

	/* (must be called on server) add to stacks of an effect */
	UFUNCTION(BlueprintCallable, reliable, NetMulticast, Category = Abilities)
	void SetEffectStacks(const FString& key, int32 newAmt, bool bShouldResetEffectTimer = true);

	/* upgrade the outfit across all clients */
	UFUNCTION(BlueprintCallable, Category=Outfit)
	void EquipOutfit(AOutfit* newOutfit);

	/* upgrade the outfit across all clients */
	UFUNCTION(BlueprintCallable, reliable, NetMulticast, Category = Outfit)
	void UpgradeOutfit(uint8 tree, uint8 row, uint8 col);

	/* gets the replicated rotator before we try other methods */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	FRotator GetAbilityControlRotation() const;
};

