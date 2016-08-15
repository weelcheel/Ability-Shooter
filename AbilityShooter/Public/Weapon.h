#pragma once

#include "EquipmentItem.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	BulletGun UMETA(DisplayName = "Bullet Firing Gun"),
	Sword UMETA(DisplayName = "Sword"),
	PA UMETA(DisplayName = "Physical Augmenter"),
	MAX UMETA(Hidden)
};

UCLASS(ABSTRACT, Blueprintable)
class AWeapon : public AEquipmentItem
{
	GENERATED_BODY()

public:

	/* weapon type */
	EWeaponType weaponType;

	AWeapon();
};