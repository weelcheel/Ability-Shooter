#pragma once

#include "EquipmentItem.generated.h"

class AAbilityShooterCharacter;
class USoundCue;

UENUM(BlueprintType)
enum class EEquipmentState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Using UMETA(DisplayName = "Using"),
	Equipping UMETA(DisplayName = "Equipping"),
	Reloading UMETA(DisplayName = "Reloading"),
	Cooldown UMETA(DisplayName = "On Cooldown"),
	MAX UMETA(Hidden)
};

UCLASS(ABSTRACT, Blueprintable)
class AEquipmentItem : public AActor
{
	GENERATED_BODY()

protected:

	/* ---------------------------------------------------------------------- */
	// Variables

	/** character owner */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_characterOwner)
	class AAbilityShooterCharacter* characterOwner;

	/** equipment mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* mesh;

	/** using audio (bLoopedUseSound set) */
	UPROPERTY(Transient)
	UAudioComponent* usingAC;

	/** single use sound (bLoopedUseSound not set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* useSound;

	/** looped use sound (bLoopedUseSound set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* useLoopSound;

	/** finished burst sound (bLoopedUseSound set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* useFinishSound;

	/** animation played on pawn when equipping (3rd person view) */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* equipAnim;

	/** equip sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* equipSound;

	/** animations played on pawn when using */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* useAnim;

	/** is use sound looped? */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	uint32 bLoopedUseSound : 1;

	/** is use animation looped for the mode? */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	uint32 bLoopedUseAnim : 1;

	/** is use animation playing? */
	uint32 bPlayingUseAnim : 1;

	/** is equipment currently equipped? */
	uint32 bIsEquipped : 1;

	/** is equipment use active? */
	uint32 bWantsToUse : 1;

	/** is equip animation playing? */
	uint32 bPendingEquip : 1;

	/* are we currently using alt? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_AltToggle)
	uint32 bWantsToAlt : 1;

	/** weapon is reusing*/
	uint32 bReusing;

	/** current weapon state */
	EEquipmentState currentState;

	/** time of last successful equipment use */
	float lastUseTime;

	/** last time when this equipment was switched to */
	float equipStartedTime;

	/** how much time equipment needs to be equipped */
	float equipDuration;

	/* how much time it takes between uses for a mode <= 0 means that it only uses once per click */
	UPROPERTY(EditDefaultsOnly, Category = Equipment)
	float timesBetweenUse;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 burstCounter;

	/** handle for efficient management of OnEquipFinished timer */
	FTimerHandle onEquipFinishedTimer;

	/** handle for efficient management of HandleFiring timer */
	FTimerHandle handleFiringTimer;

	/** perform initial setup */
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartUse();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopUse();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartAlt();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopAlt();

	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_characterOwner();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_AltToggle();

	/** Called in network play to do the cosmetic fx for using */
	virtual void SimulateEquipmentUse();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingEquipmentUse();

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] equipment specific fire implementation */
	virtual void UseEquipment() PURE_VIRTUAL(AEquipmentItem::UseEquipment, );

	/** [server] use & update any stats needed */
	UFUNCTION(reliable, server, WithValidation)
	void ServerHandleUsing();

	/** [local + server] handle equipment use */
	virtual void HandleUsing();

	/** [local + server] using started */
	virtual void OnBurstStarted();

	/** [local + server] using finished */
	virtual void OnBurstFinished();

	/** [local + server] alt started */
	virtual void OnAltStarted();

	/** [local + server] alt finished */
	virtual void OnAltFinished();

	/** update equipment state */
	void SetEquipmentState(EEquipmentState newState);

	/** determine current equipment state */
	virtual void DetermineEquipmentState();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** attaches equipment mesh to character's mesh */
	void AttachMeshToCharacter();

	/** detaches equipment mesh to character's mesh */
	void DetachMeshFromCharacter();

	/** weapon is now equipped by owner pawn */
	virtual void OnEquipFinished();

	/** check if it's currently equipped */
	bool IsEquipped() const;

	/** check if mesh is already attached */
	bool IsAttachedToCharacter() const;

	//////////////////////////////////////////////////////////////////////////
	// Control

	/** check if equipment can be used */
	virtual bool CanUse() const;

	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get current equipment state */
	EEquipmentState GetCurrentState() const;

	/** get equipment mesh */
	USkeletalMeshComponent* GetEquipmentMesh() const;

	/* @TODO: equipment HUD icons and crosshairs */

	/** gets last time when this weapon was switched to */
	float GetEquipStartedTime() const;

	/** gets the duration of equipping weapon*/
	float GetEquipDuration() const;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** play weapon sounds */
	UAudioComponent* PlayEquipmentSound(USoundCue* sound);

	/** play equipment animations */
	float PlayEquipmentAnimation(UAnimMontage* animation);

	/** stop playing weapon animations */
	void StopEquipmentAnimation(UAnimMontage* animation);

	/** Get the aim of the equipment, allowing for adjustments to be made by the equipment */
	virtual FVector GetAdjustedAim() const;

	/** Get the aim of the camera */
	FVector GetCameraAim() const;

	/** get the originating location for camera damage */
	FVector GetCameraDamageStartLocation(const FVector& AimDir) const;

	/** find hit */
	FHitResult EquipmentTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

public:
	AEquipmentItem();

	/* gets the mesh of the equipment */
	USkeletalMeshComponent* GetMesh() const;

	/** weapon is being equipped by owner pawn */
	virtual void OnEquip(const AEquipmentItem* lastItem);

	/** weapon is holstered by owner pawn */
	virtual void OnUnEquip();

	/** [server] weapon was added to pawn's inventory */
	virtual void OnEnterInventory(AAbilityShooterCharacter* newOwner);

	/** [server] weapon was removed from pawn's inventory */
	virtual void OnLeaveInventory();

	/** get character owner */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	AAbilityShooterCharacter* GetCharacterOwner() const;

	/** set the weapon's owning pawn */
	void SetOwningCharacter(AAbilityShooterCharacter* newOwner);

	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] start equipment use */
	virtual void StartUse();

	/** [local + server] stop equipment use */
	virtual void StopUse();

	/** [local + server] start equipment use */
	virtual void StartAlt();

	/** [local + server] stop equipment use */
	virtual void StopAlt();
};