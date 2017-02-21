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

void AOutfit::EquipOutfit(AAbilityShooterCharacter* character, bool bReactivate)
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

	if (bReactivate)
	{
		for (int32 i = 0; i < offenseUpgradeTree.Num(); i++)
		{
			for (int32 j = 0; j < offenseUpgradeTree[i].row.Num(); j++)
			{
				if (offenseUpgradeTree[i].row[j].bIsUpgradeActive && IsValid(offenseUpgradeTree[i].row[j].spawnedUpgrade))
					offenseUpgradeTree[i].row[j].spawnedUpgrade->Reactivate(owningCharacter, this);
			}
		}
	}

	OnEquipped(character);
}

void AOutfit::Upgrade(uint8 tree, uint8 row, uint8 col)
{
	//check other upgrades to see if this upgrade can be activated
	//check to make sure no other upgrades in this row are active
	switch (tree)
	{
	case 0:
		for (int32 i = 0; i < offenseUpgradeTree[row].row.Num(); i++)
		{
			if (i != col && offenseUpgradeTree[row].row[i].bIsUpgradeActive)
				return;
		}
		break;
	case 1:
		for (int32 i = 0; i < defenseUpgradeTree[row].row.Num(); i++)
		{
			if (i != col && defenseUpgradeTree[row].row[i].bIsUpgradeActive)
				return;
		}
		break;
	case 2:
		for (int32 i = 0; i < utilityUpgradeTree[row].row.Num(); i++)
		{
			if (i != col && utilityUpgradeTree[row].row[i].bIsUpgradeActive)
				return;
		}
		break;
	}

	//check to make sure the row before this one has an upgrade, unless the target row is the first row
	if (row > 0)
	{
		bool bFoundUpgrade = false;
		switch (tree)
		{
		case 0:
			for (int32 i = 0; i < offenseUpgradeTree[row-1].row.Num(); i++)
			{
				if (offenseUpgradeTree[row-1].row[i].bIsUpgradeActive)
					bFoundUpgrade = true;
			}
			break;
		case 1:
			for (int32 i = 0; i < defenseUpgradeTree[row-1].row.Num(); i++)
			{
				if (defenseUpgradeTree[row-1].row[i].bIsUpgradeActive)
					bFoundUpgrade = true;
			}
			break;
		case 2:
			for (int32 i = 0; i < utilityUpgradeTree[row-1].row.Num(); i++)
			{
				if (utilityUpgradeTree[row-1].row[i].bIsUpgradeActive)
					bFoundUpgrade = true;
			}
			break;
		}

		if (!bFoundUpgrade)
			return;
	}

	//activate the upgrade
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
					offenseUpgradeTree[row].row[col].bIsUpgradeActive = true;
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
					defenseUpgradeTree[row].row[col].bIsUpgradeActive = true;
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
					utilityUpgradeTree[row].row[col].bIsUpgradeActive = true;
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
	/*for (int32 row = 0; row < offenseUpgradeTree.Num(); row++)
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
	}*/

	return deltaStats;
}