#pragma once

#include "ShooterItem.h"
#include "StatsManager.h"
#include "ShooterDamage.h"
#include "OutfitUpgrade.generated.h"

class AAbilityShooterCharacter;
class AAbility;
class AOutfit;

UCLASS(ABSTRACT, Blueprintable)
class AOutfitUpgrade : public AShooterItem
{
	friend class AOutfit;

	GENERATED_BODY()

protected:

	/* stats this upgrade gives */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stats)
	FBaseStats stats;

	/* abilities this upgrade gives */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Abilities)
	TArray<TSubclassOf<AAbility> > abilities;

	/* owner of the outfit */
	UPROPERTY(BlueprintReadOnly, Category = Owner)
	AAbilityShooterCharacter* owningCharacter;

	/* blueprint hook for when this upgrade is activated */
	UFUNCTION(BlueprintImplementableEvent, Category = Activated)
	void OnActivated(AAbilityShooterCharacter* upgrader, AOutfit* owningOutfit);

public:
	AOutfitUpgrade();

	/* called when this upgrade is activated */
	void Activate(AAbilityShooterCharacter* upgrader, AOutfit* owningOutfit);

	/* reactivate the upgrade to re add abilities and effects after the owning character has been reswpawned while owning the outfit */
	void Reactivate(AAbilityShooterCharacter* upgrader, AOutfit* owningOutfit);
};