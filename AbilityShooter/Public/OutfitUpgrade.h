#pragma once

#include "ShooterItem.h"
#include "OutfitUpgrade.generated.h"

UCLASS(ABSTRACT, Blueprintable)
class AOutfitUpgrade : public AShooterItem
{
	GENERATED_BODY()

public:
	AOutfitUpgrade();
};