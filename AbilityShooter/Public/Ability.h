#pragma once

#include "AbilityShooterCharacter.h"
#include "AbilityShooterTypes.h"
#include "Ability.generated.h"

UENUM()
enum class EAbilityState : uint8
{
	NoOwner,
	Idle,
	Disabled,
	OnCooldown,
	Aiming,
	Performing,
	MAX
};

UCLASS()
class AAbility : public AActor
{
	friend class AAbilityShooterCharacter;

	GENERATED_BODY()

protected:

	/* what current state the ability is in */
	EAbilityState currentState;

	/** character owner */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing=OnRep_CharacterOwner, Category = Ability)
	AAbilityShooterCharacter* characterOwner;

	/* what veteran level this ability is */
	UPROPERTY(replicated, BlueprintReadOnly, Category = Ability)
	int32 veteranLevel;

	/* max veteran level this ability can have */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Ability)
	int32 maxVeteranLevel;

	/* timer for cooldowns */
	UPROPERTY(BlueprintReadOnly, Category=Cooldown)
	FTimerHandle cooldownTimer;

	/* next state to go to after cooldown is finished */
	UPROPERTY(BlueprintReadOnly, Category = Cooldown)
	EAbilityState afterCooldownState;

	/* next manual cooldown time to use */
	UPROPERTY(BlueprintReadOnly, Category = Cooldown)
	float manualCooldownTime;

	/* whether or not this Ability automatically enters the performing state on use */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bAutoPerform;

	/* whether or not this Ability is an Ultimate Ability */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bUltimateAbility;

	/* whether or not this Ability has an aiming state available */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bHasAimingState;

	/* whether or not this ability makes the character face the camera's rotation */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bPerformingRotatesOwnerWithAim;

	/* whether or not this ability stops performing when the button is released */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bStopPerformingWhenButtonIsReleased;

	/* whether or not to disable look input when this ability performs */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bIgnoreLookInputWhilePerforming;

	/* whether or not to disable movement when this ability performs */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bIgnoreMovementWhilePerforming;

	/* whether or not this ability stops all other abilities on use */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bShouldStopAllOtherAbilitiesOnUse;

	/* whether or not this ability disables all other abilities on use */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bShouldDisableAllOtherAbilitiesOnUse;

	/* replicated boolean to give performing effects */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_IsPerforming)
	bool bIsPerforming;

	/* replicated boolean to give aiming effects */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_IsAiming)
	bool bIsAiming;

	/** is Ability use active? */
	uint32 bWantsToPerform : 1;

	/** Ability wants to enter cooldown? */
	uint32 bWantsToCooldown : 1;

	/** Ability wants to confirm aiming? */
	uint32 bWantsToConfirmAim : 1;

	/* array of cooldowns for each veteran level available */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TArray<float> baseCooldownTimes;

	/* name of this Ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ability)
	FText title;

	/* description of the Ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ability)
	FText description;

	/* amount of time (if any) this Ability takes as a Character Action before performing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ability)
	FCharacterActionInfo executionTimeInfo;
	
	/* timer for execution */
	FTimerHandle executionTimer;

	/* blueprint hooks for unique logic */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void Perform();

	/* perform after testing whether or not this ability should be performed */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void ContinueHandlePerform();

	/* if the ability can't auto perform, then determine if it should perform at all */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void ShouldPerform();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void OnAbilityStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void OnAimingStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void OnAimingStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void StartPerformEffects();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void StopPerformEffects();

	/* server start perform */
	UFUNCTION(reliable, server, WithValidation)
	void ServerStartPerform();

	/* server stop perform */
	UFUNCTION(reliable, server, WithValidation)
	void ServerStopPerform(bool bFromInput = false);

	/* server handle perform */
	UFUNCTION(reliable, server, WithValidation)
	void ServerHandlePerform();

	/* server confirm aim */
	UFUNCTION(reliable, server, WithValidation)
	void ServerConfirmAim();

	/* called to get whether or not this ability can be performed */
	bool CanPerform() const;

	/* called when cooldown is finished */
	void CooldownFinished();

	/* determines the next state for this Ability */
	void DetermineState();

	/* sets the state and begins or ends performance as necessary */
	void SetState(EAbilityState newState);

	/* handles actually performing the ability */
	void HandlePerform();

	/* on stop performing */
	void OnStopPerform();

	/* actually sets the cooldown timer for this ability */
	void StartCooldownTimer();

	/* perform a trace for an ability */
	UFUNCTION(BlueprintCallable, Category = Ability)
	FHitResult AbilityTrace(UPARAM(ref) const FVector& from, UPARAM(ref) const FVector& to) const;

	/* get the aim for the ability */
	UFUNCTION(BlueprintCallable, Category = Ability)
	FVector GetAdjustedAim() const;

	/* get the start trace location */
	UFUNCTION(BlueprintCallable, Category = Ability)
	FVector GetCameraDamageStartLocation(UPARAM(ref) const FVector& aimDir) const;

	/* launch projectile from the locally controlled character */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void LaunchProjectile(UPARAM(ref) const FName& originBone, TSubclassOf<AProjectile> projectileType);

	/* launch the projectile on the server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerLaunchProjectile(const FVector& origin, const FVector& launchDir, TSubclassOf<AProjectile> projectileType);

	/* server start execution timer */
	UFUNCTION(reliable, server, WithValidation, BlueprintCallable, Category = CharacterAction)
	void ServerStartExecutionTimer();

	/* whether or not this ability can deal damage to a certain character */
	UFUNCTION(BlueprintCallable, Category = Ability)
	bool CanHurtCharacter(AAbilityShooterCharacter* testCharacter) const;

	UFUNCTION()
	void OnRep_CharacterOwner();

	UFUNCTION()
	void OnRep_IsPerforming();

	UFUNCTION()
	void OnRep_IsAiming();

public:
	AAbility();

	/* called locally whenever this Ability is starting to be performed (the Ability button is pushed down) */
	void StartPerform();

	/* called locally whenever this Ability is stopped performing (the Ability button is let go) */
	void StopPerform(bool bFromInput = false);

	/* called locally to confirm the aim of this Ability (the player pressed the Primary button while aiming this ability) */
	void ConfirmAim();

	/* called to setup the ability to a purchasing owner */
	void SetupAbility(AAbilityShooterCharacter* newOwner);

	/* starts the ability's cooldown on all clients when called from server */
	UFUNCTION(BlueprintCallable, reliable, NetMulticast, Category = Cooldown)
	void StartCooldown(float manualCooldown = -1.f, EAbilityState cdfState = EAbilityState::Idle);

	/* server start cooldown */
	UFUNCTION(BlueprintCallable, reliable, server, WithValidation, Category = Cooldown)
	void ServerStartCooldown(float manualCooldown = -1.f, EAbilityState cdfState = EAbilityState::Idle);

	/* [SERVER] add a veteran level */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void AddVeteranLevel();

	/* gets a value scaled by this Ability's level from a table of values */
	UFUNCTION(BlueprintCallable, Category = Ability)
	float GetVeteranLevelScaledValue(UPARAM(ref) TArray<float>& values) const;

	/* gets the percentage of cooldown progress */
	UFUNCTION(BlueprintCallable, Category = Cooldown)
	float GetCooldownProgressPercent() const;

	/* gets the amount of time left in the cooldown */
	UFUNCTION(BlueprintCallable, Category = Cooldown)
	float GetCooldownRemaining() const;

	/* performs a psuedo cone trace for the Ability. Really just a box trace where we check the angles of the hit objects */
	UFUNCTION(BlueprintCallable, Category = Trace)
	void ConeTrace(FVector& start, FVector& end, float boxSize, TArray<FHitResult>& outHits);

	/* interrupt handler */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void HandleInterrupt(EAbilityInterruptSignal signal);

	/* make sure we call interrupts on the server because they can be caught on the client (changing directions, etc.) */
	UFUNCTION(reliable, server, WithValidation)
	void ServerReceiveInterrupt(EAbilityInterruptSignal signal);

	/* handle ability interrupts while performing */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void AbilityReceivedInterruptSignal(EAbilityInterruptSignal signal);

	/* force this ability to stop performing on all clients and server */
	UFUNCTION(NetMulticast, reliable, BlueprintCallable, Category = Ability)
	void ForceStopAbility();

	/* set whether or not this ability is disabeld */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void SetDisabled(bool bDisabled = false);

	/* gets the Ability state */
	UFUNCTION(BlueprintCallable, Category = Ability)
	EAbilityState GetCurrentState() const
	{
		return currentState;
	}

	/* gets the Ability's name */
	UFUNCTION(BlueprintCallable, Category = Ability)
	FText GetTitle() const
	{
		return title;
	}

	/* gets the Ability's desc */
	UFUNCTION(BlueprintCallable, Category = Ability)
	FText GetDescription() const
	{
		return description;
	}
};