#include "AbilityShooter.h"
#include "StatsManager.h"
#include "Effect.h"
#include "AbilityShooterCharacter.h"

UStatsManager::UStatsManager()
{

}

void UStatsManager::SetOwningCharacter(AAbilityShooterCharacter* newOwner)
{
	characterOwner = newOwner;
}

float UStatsManager::GetCurrentStatValue(EStat stat)
{
	float statn = -1.f;

	if (!IsValid(characterOwner))
		return statn;

	//first get the base stat
	statn = GetBaseStatValue(stat);

	//@TODO: next, let the stat go through the current outfit to be altered

	return statn;
}

float UStatsManager::GetBaseStatValue(EStat stat)
{
	float statn = -1.f;

	if (!IsValid(characterOwner))
		return statn;

	//first get the base stat
	switch (stat)
	{
	case EStat::ES_Atk:
		statn = characterOwner->baseStats.attack;
		break;
	case EStat::ES_Def:
		statn = characterOwner->baseStats.defense;
		break;
	case EStat::ES_SpAtk:
		statn = characterOwner->baseStats.specialAttack;
		break;
	case EStat::ES_SpDef:
		statn = characterOwner->baseStats.specialDefense;
		break;
	case EStat::ES_EquipUseRate:
		statn = characterOwner->baseStats.equipUseRate;
		break;
	case EStat::ES_Move:
		statn = characterOwner->baseStats.movementSpeed;
		break;
	case EStat::ES_CDR:
		statn = characterOwner->baseStats.cooldownReduction;
		break;
	case EStat::ES_HP:
		statn = characterOwner->baseStats.HP;
		break;
	case EStat::ES_CritChance:
		statn = characterOwner->baseStats.critChance;
		break;
	case EStat::ES_SpecialCritChance:
		statn = characterOwner->baseStats.specialCritChance;
		break;
	case EStat::ES_CritRatio:
		statn = characterOwner->baseStats.critRatio;
		break;
	case EStat::ES_Acc:
		statn = characterOwner->baseStats.accuracy;
		break;
	case EStat::ES_Eva:
		statn = characterOwner->baseStats.evasiveness;
		break;
	case EStat::ES_HPDrain:
		statn = characterOwner->baseStats.healthDrain;
		break;
	}

	return statn;
}