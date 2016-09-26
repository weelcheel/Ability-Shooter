#pragma once

#include "AIController.h"
#include "TrainingDummyController.generated.h"

class AAbilityShooterCharacter;

UCLASS()
class ATrainingDummyController : public AAIController
{
	GENERATED_BODY()

protected:

	/* casted owned character */
	UPROPERTY(BlueprintReadOnly, Category = Character)
	AAbilityShooterCharacter* ownedCharacter;

	/* start primary equipment use */
	UFUNCTION(BlueprintCallable, Category = Equipment)
	void StartEquipmentUse();

	/* stop primary equipment use */
	UFUNCTION(BlueprintCallable, Category = Equipment)
	void StopEquipmentUse();

	/* start alternate equipment use */
	UFUNCTION(BlueprintCallable, Category = Equipment)
	void StartEquipmentAltUse();

	/* stop primary equipment use */
	UFUNCTION(BlueprintCallable, Category = Equipment)
	void StopEquipmentAltUse();

	/* start ability use */
	UFUNCTION(BlueprintCallable, Category = Equipment)
	void StartPerformAbility(int32 abilityIndex);

	/* stop ability use */
	UFUNCTION(BlueprintCallable, Category = Equipment)
	void StopPerformAbility(int32 abilityIndex);

public:
	ATrainingDummyController();

	virtual void Possess(APawn* InPawn) override;
};
