#include "AbilityShooter.h"
#include "OutfitUpgrade.h"
#include "Ability.h"
#include "Outfit.h"

AOutfitUpgrade::AOutfitUpgrade()
{
	stats.equipUseRate = 0.f;
	stats.critRatio = 0.f;
}

void AOutfitUpgrade::Activate(AAbilityShooterCharacter* upgrader, AOutfit* owningOutfit)
{
	//add stats to outfit
	if (IsValid(owningOutfit))
	{
		owningOutfit->stats += stats;
	}

	//give character abilities and get event hooks
	if (IsValid(upgrader))
	{
		for (TSubclassOf<AAbility> ability : abilities)
		{
			upgrader->AddAbility(ability, true);
		}

		//add event hooks for the Shooter to broadcast to this upgrade
		upgrader->OnShooterDamaged.AddDynamic(this, &AOutfitUpgrade::OnOwnerDamaged);
		upgrader->OnShooterDealtDamage.AddDynamic(this, &AOutfitUpgrade::OnOwnerDealtDamage);
	}

	owningCharacter = upgrader;

	OnActivated(upgrader, owningOutfit);
}