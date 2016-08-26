#pragma once

#include "Ability.generated.h"

UENUM()
enum class EAbilityState : uint8
{
	NoOwner,
	Idle,
	Disabled,
	OnCooldown,
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
	UPROPERTY(Transient, replicated)
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

	/* whether or not this Ability automatically enters the performing state on use */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bAutoPerform;

	/** is Ability use active? */
	uint32 bWantsToPerform : 1;

	/* array of cooldowns for each veteran level available */
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TArray<float> baseCooldownTimes;

	/* name of this Ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ability)
	FText title;

	/* description of the Ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ability)
	FText description;

	/* called to get whether or not this ability can be performed */
	bool CanPerform() const;

	/* called when cooldown is finished */
	void CooldownFinished();

	/* determines the next state for this Ability */
	void DetermineState();

	/* sets the state and begins or ends performance as necessary */
	void SetState(EAbilityState newState);

public:
	AAbility();

	/* called locally whenever this Ability is starting to be performed (the Ability button is pushed down) */
	void StartPerform();

	/* called locally whenever this Ability is stopped performing (the Ability button is let go) */
	void StopPerform();

	/* blueprint hooks for unique logic */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void OnStartPerform();

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	void OnStopPerform();

	/* server start perform */
	UFUNCTION(reliable, server, WithValidation)
	void ServerStartPerform();

	/* server stop perform */
	UFUNCTION(reliable, server, WithValidation)
	void ServerStopPerform();

	/* called to setup the ability to a purchasing owner */
	void SetupAbility(AAbilityShooterCharacter* newOwner);

	/* starts the ability's cooldown on all clients when called from server */
	UFUNCTION(BlueprintCallable, reliable, NetMulticast, Category = Cooldown)
	void StartCooldown(float manualCooldown = -1.f, EAbilityState cdfState = EAbilityState::Idle);

	/* [SERVER] add a veteran level */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void AddVeteranLevel();

	/* gets a value scaled by this Ability's level from a table of values */
	UFUNCTION(BlueprintCallable, Category = Ability)
	float GetVeteranLevelScaledValue(TArray<float>& values) const;

	/* gets the percentage of cooldown progress */
	UFUNCTION(BlueprintCallable, Category = Cooldown)
	float GetCooldownProgressPercent() const;

	/* gets the amount of time left in the cooldown */
	UFUNCTION(BlueprintCallable, Category = Cooldown)
	float GetCooldownRemaining() const;

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