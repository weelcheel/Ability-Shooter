#include "AbilityShooter.h"
#include "Outfit.h"
#include "AbilityShooterCharacter.h"
#include "OutfitUpgrade.h"
#include "StatsManager.h"
#include "Ability.h"

AOutfit::AOutfit()
{
	stats.equipUseRate = 0.f;
	stats.critRatio = 0.f;
}

void AOutfit::EquipOutfit(AAbilityShooterCharacter* character)
{
	owningCharacter = character;

	if (IsValid(owningCharacter))
	{
		for (TSubclassOf<AAbility> abType : abilities)
		{
			owningCharacter->AddAbility(abType, true);
		}

		FShooterDamagedDelegate damageEvent;
		damageEvent.BindUObject(this, &AOutfit::OnOwnerDamaged);
		owningCharacter->OnShooterDamagedEvents.Add(damageEvent);

		FShooterDealtDamageDelegate damagedEvent;
		damagedEvent.BindUObject(this, &AOutfit::OnOwnerDealtDamage);
		owningCharacter->OnShooterDealtDamageEvents.Add(damagedEvent);

		//owningCharacter->OnShooterDamaged.BindDynamic(this, &AOutfit::OnOwnerDamaged);
		//owningCharacter->OnShooterDealtDamage.BindDynamic(this, &AOutfit::OnOwnerDealtDamage);
	}

	OnEquipped(character);
}

void AOutfit::Upgrade(uint8 tree, uint8 row, uint8 col)
{
	switch (tree)
	{
	case 0: //offense
		if (row >= 0 && row <= offenseUpgradeTree.Num() && col >= 0 && col < offenseUpgradeTree[row].row.Num())
		{
			if (IsValid(owningCharacter) && offenseUpgradeTree[row].row[col].ShouldActivate())
			{
				offenseUpgradeTree[row].row[col].spawnedUpgrade = GetWorld()->SpawnActor<AOutfitUpgrade>(offenseUpgradeTree[row].row[col].upgradeItem.itemType);
				if (IsValid(offenseUpgradeTree[row].row[col].spawnedUpgrade))
				{
					offenseUpgradeTree[row].row[col].spawnedUpgrade->Activate(owningCharacter, this);
				}
			}
		}
		break;
	case 1: //defense
		if (row >= 0 && row <= defenseUpgradeTree.Num() && col >= 0 && col < defenseUpgradeTree[row].row.Num())
		{
			if (IsValid(owningCharacter) && defenseUpgradeTree[row].row[col].ShouldActivate())
			{
				defenseUpgradeTree[row].row[col].spawnedUpgrade = GetWorld()->SpawnActor<AOutfitUpgrade>(defenseUpgradeTree[row].row[col].upgradeItem.itemType);
				if (IsValid(defenseUpgradeTree[row].row[col].spawnedUpgrade))
				{
					defenseUpgradeTree[row].row[col].spawnedUpgrade->Activate(owningCharacter, this);
				}
			}
		}
		break;
	case 2: //utility
		if (row >= 0 && row <= utilityUpgradeTree.Num() && col >= 0 && col < utilityUpgradeTree[row].row.Num())
		{
			if (IsValid(owningCharacter) && utilityUpgradeTree[row].row[col].ShouldActivate())
			{
				utilityUpgradeTree[row].row[col].spawnedUpgrade = GetWorld()->SpawnActor<AOutfitUpgrade>(utilityUpgradeTree[row].row[col].upgradeItem.itemType);
				if (IsValid(utilityUpgradeTree[row].row[col].spawnedUpgrade))
				{
					utilityUpgradeTree[row].row[col].spawnedUpgrade->Activate(owningCharacter, this);
				}
			}
		}
		break;
	}
}

FBaseStats AOutfit::GetDeltaStats()
{
	FBaseStats deltaStats = stats;
	//offense upgrades
	for (int32 row = 0; row < offenseUpgradeTree.Num(); row++)
	{
		for (int32 col = 0; col < offenseUpgradeTree[row].row.Num(); col++)
		{
			if (offenseUpgradeTree[row].row[col].bIsUpgradeActive && IsValid(offenseUpgradeTree[row].row[col].spawnedUpgrade))
			{
				deltaStats += offenseUpgradeTree[row].row[col].spawnedUpgrade->stats;
			}
		}
	}
	//defense upgrades
	for (int32 row = 0; row < defenseUpgradeTree.Num(); row++)
	{
		for (int32 col = 0; col < defenseUpgradeTree[row].row.Num(); col++)
		{
			if (defenseUpgradeTree[row].row[col].bIsUpgradeActive && IsValid(defenseUpgradeTree[row].row[col].spawnedUpgrade))
			{
				deltaStats += defenseUpgradeTree[row].row[col].spawnedUpgrade->stats;
			}
		}
	}
	//utility tree
	for (int32 row = 0; row < utilityUpgradeTree.Num(); row++)
	{
		for (int32 col = 0; col < utilityUpgradeTree[row].row.Num(); col++)
		{
			if (utilityUpgradeTree[row].row[col].bIsUpgradeActive && IsValid(utilityUpgradeTree[row].row[col].spawnedUpgrade))
			{
				deltaStats += utilityUpgradeTree[row].row[col].spawnedUpgrade->stats;
			}
		}
	}

	return deltaStats;
}