#pragma once

#include "ShooterItem.h"
#include "StoreItem.h"
#include "OutfitUpgrade.h"
#include "Outfit.generated.h"

USTRUCT(Blueprintable)
struct FOutfitUpgradeSlot
{
	GENERATED_USTRUCT_BODY()

	/* upgrade to fill the slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UpgradeSlot)
	FStoreItem upgradeItem;

	/* whether or not this upgrade is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UpgradeSlot)
	bool bIsUpgradeActive = false;
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

public:
	AOutfit();
};