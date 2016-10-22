#include "AbilityShooter.h"
#include "OutfitUpgrade.h"
#include "Ability.h"
#include "Outfit.h"

AOutfitUpgrade::AOutfitUpgrade()
{

}

void AOutfitUpgrade::Activate(AAbilityShooterCharacter* upgrader, AOutfit* owningOutfit)
{
	//add stats to outfit
	if (IsValid(owningOutfit))
	{
		owningOutfit->stats += stats;
	}

	//give character abilities
	if (IsValid(upgrader))
	{
		for (TSubclassOf<AAbility> ability : abilities)
		{
			upgrader->AddAbility(ability, true);
		}
	}

	OnActivated(upgrader, owningOutfit);
}