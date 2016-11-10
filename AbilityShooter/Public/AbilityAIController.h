#pragma once

#include "AIController.h"
#include "AbilityAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class AAbilityShooterCharacter;
class AEquipmentItem;
class AAbility;

struct FEffectInitInfo;

UCLASS()
class AAbilityAIController : public AAIController
{
	GENERATED_BODY()

private:
	UPROPERTY(transient)
		UBlackboardComponent* BlackboardComp;

	/* Cached BT component */
	UPROPERTY(transient)
		UBehaviorTreeComponent* BehaviorComp;

protected:
	// Check of we have LOS to a character
	bool LOSTrace(AAbilityShooterCharacter* targetChar) const;

	int32 EnemyKeyID;
	int32 NeedNewEquipmentKeyID;

	/** Handle for efficient management of Respawn timer */
	FTimerHandle TimerHandle_Respawn;

public:
	AAbilityAIController();

	// Begin AController interface
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
	virtual void Possess(class APawn* InPawn) override;
	virtual void UnPossess() override;
	virtual void BeginInactiveState() override;
	// End APlayerController interface

	/* caches the persisting effects the player has to apply to the new character on spawn */
	TArray<FEffectInitInfo> persistentEffects;

	/* caches the abilites the player has to give to the new character on spawn */
	TArray<AAbility*> persistentAbilities;

	void Respawn();

	void CheckCanUseEquipment(const class AEquipmentItem* currentEquipment);

	void SetTarget(class APawn* InPawn);

	class AAbilityShooterCharacter* GetTarget() const;

	/* If there is line of sight to current enemy, start firing at them */
	UFUNCTION(BlueprintCallable, Category = Behavior)
	void AttackTarget();

	/* Finds the closest enemy and sets them as current target */
	UFUNCTION(BlueprintCallable, Category = Behavior)
	void FindClosestEnemy();

	UFUNCTION(BlueprintCallable, Category = Behavior)
	bool FindClosestEnemyWithLOS(AAbilityShooterCharacter* ExcludeEnemy);

	bool HasEquipmentLOSToTarget(AActor* targetActor, const bool bAnyEnemy);

	// Begin AAIController interface
	/** Update direction AI is looking based on FocalPoint */
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	// End AAIController interface

	/** Returns BlackboardComp subobject **/
	FORCEINLINE UBlackboardComponent* GetBlackboardComp() const { return BlackboardComp; }
	/** Returns BehaviorComp subobject **/
	FORCEINLINE UBehaviorTreeComponent* GetBehaviorComp() const { return BehaviorComp; }
};