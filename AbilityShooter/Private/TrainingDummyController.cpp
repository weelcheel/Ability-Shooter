#include "AbilityShooter.h"
#include "TrainingDummyController.h"
#include "AbilityShooterCharacter.h"
#include "EquipmentItem.h"
#include "Ability.h"

ATrainingDummyController::ATrainingDummyController()
{

}

void ATrainingDummyController::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	ownedCharacter = Cast<AAbilityShooterCharacter>(InPawn);
}

void ATrainingDummyController::StartEquipmentUse()
{
	if (IsValid(ownedCharacter) && IsValid(ownedCharacter->GetCurrentEquipment()))
	{
		ownedCharacter->GetCurrentEquipment()->StartUse();
	}
}

void ATrainingDummyController::StopEquipmentUse()
{
	if (IsValid(ownedCharacter) && IsValid(ownedCharacter->GetCurrentEquipment()))
	{
		ownedCharacter->GetCurrentEquipment()->StopUse();
	}
}

void ATrainingDummyController::StartEquipmentAltUse()
{
	if (IsValid(ownedCharacter) && IsValid(ownedCharacter->GetCurrentEquipment()))
	{
		ownedCharacter->GetCurrentEquipment()->StartAlt();
	}
}

void ATrainingDummyController::StopEquipmentAltUse()
{
	if (IsValid(ownedCharacter) && IsValid(ownedCharacter->GetCurrentEquipment()))
	{
		ownedCharacter->GetCurrentEquipment()->StopAlt();
	}
}

void ATrainingDummyController::StartPerformAbility(int32 abilityIndex)
{
	if (IsValid(ownedCharacter))
	{
		ownedCharacter->OnStartAbility(abilityIndex);
	}
}

void ATrainingDummyController::StopPerformAbility(int32 abilityIndex)
{
	if (IsValid(ownedCharacter))
	{
		ownedCharacter->OnStopAbility(abilityIndex);
	}
}