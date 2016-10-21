#include "AbilityShooter.h"
#include "ShooterItem.h"

AShooterItem::AShooterItem()
{
	uiIcon = nullptr;
}

UObject* AShooterItem::GetDefaultObjectForClass(TSubclassOf<UObject> type)
{
	if (IsValid(type))
	{
		return type->GetDefaultObject();
	}

	return nullptr;
}