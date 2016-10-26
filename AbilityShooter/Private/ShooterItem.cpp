#include "AbilityShooter.h"
#include "ShooterItem.h"

AShooterItem::AShooterItem()
{
	uiIcon = nullptr;

	bReplicates = true;
}

UObject* AShooterItem::GetDefaultObjectForClass(TSubclassOf<UObject> type)
{
	if (IsValid(type))
	{
		return type->GetDefaultObject();
	}

	return nullptr;
}