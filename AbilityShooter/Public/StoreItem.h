#pragma once

#include "ShooterItem.h"
#include "StoreItem.generated.h"

USTRUCT(Blueprintable)
struct FStoreItem
{
	GENERATED_USTRUCT_BODY()

	/* cost of the item for the gametype */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StoreItem)
	float cost;

	/* cost of the item for the gametype */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StoreItem)
	TSubclassOf<AShooterItem> itemType;
};