#pragma once

#include "ShooterItem.h"
#include "StoreItem.h"
#include "StatsManager.h"
#include "ShooterDamage.h"
#include "Outfit.generated.h"

class AAbilityShooterCharacter;
class AOutfitUpgrade;
class AAbility;

USTRUCT(Blueprintable)
struct FOutfitUpgradeSlot
{
	GENERATED_USTRUCT_BODY()

	/* upgrade to fill the slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UpgradeSlot)
	FStoreItem upgradeItem;

	/* spawned upgrade if active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UpgradeSlot)
	AOutfitUpgrade* spawnedUpgrade;

	/* whether or not this upgrade is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UpgradeSlot)
	bool bIsUpgradeActive = false;

	bool ShouldActivate() const
	{
		return !bIsUpgradeActive;
	}
};

USTRUCT(Blueprintable)
struct FOutfitUpgradeRow
{
	GENERATED_USTRUCT_BODY()

	/* list of upgrade slots for this row */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UpgradeRow)
	TArray<FOutfitUpgradeSlot> row;
};

UCLASS(ABSTRACT, Blueprintable)
class AOutfit : public AShooterItem
{
	friend class AOutfitUpgrade;

	GENERATED_BODY()

protected:

	/* upgrade tree for offense */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UpgradeTree)
	TArray<FOutfitUpgradeRow> offenseUpgradeTree;

	/* upgrade tree for defense */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UpgradeTree)
	TArray<FOutfitUpgradeRow> defenseUpgradeTree;

	/* upgrade tree for utility */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UpgradeTree)
	TArray<FOutfitUpgradeRow> utilityUpgradeTree;

	/* array of abilities that this outfit gives to the owning character (this and upgrades can only give max 4 abilities) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Abilities)
	TArray<TSubclassOf<AAbility> > abilities;

	/* owner of the outfit */
	UPROPERTY(BlueprintReadOnly, Category = Owner)
	AAbilityShooterCharacter* owningCharacter;

	/* blueprint hook for when the outfit is equipped */
	UFUNCTION(BlueprintImplementableEvent, Category = Equip)
	void OnEquipped(AAbilityShooterCharacter* newOwner);

public:
	AOutfit();

	/* stats that this outfit affects for the character wearing it. change with upgrades */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stats)
	FBaseStats stats;

	/* called when the outfit is equipped to a Shooter */
	void EquipOutfit(AAbilityShooterCharacter* character, bool bReactivate = false);

	/* upgrade the outfit */
	void Upgrade(uint8 tree, uint8 row, uint8 col);

	/* generate a stats struct that includes delta stats of this outfit and all of its activated upgrades */
	FBaseStats GetDeltaStats();
};