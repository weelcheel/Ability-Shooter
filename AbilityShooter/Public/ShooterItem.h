#pragma once

#include "ShooterDamage.h"
#include "ShooterItem.generated.h"

class AAbilityShooterCharacter;

UCLASS(ABSTRACT, Blueprintable)
class AShooterItem : public AActor
{
	GENERATED_BODY()

protected:

	/* ui name of this item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	FText uiName;

	/* description of the item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	FText uiDescription;

	/* ui icon for this item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	UTexture2D* uiIcon;

	/* ===============================Event Handlers=========================================*/

	/* whenever the owner takes damage */
	UFUNCTION(BlueprintImplementableEvent, Category = Damage)
	void OnOwnerDamaged(FShooterDamage damage);

	/* whenever the owner deals damage */
	UFUNCTION(BlueprintImplementableEvent, Category = Damage)
	void OnOwnerDealtDamage(FShooterDamage damage, AAbilityShooterCharacter* damagedCharacter);

public:
	AShooterItem();

	/* static function for getting default objects in blueprints */
	UFUNCTION(BlueprintCallable, Category = DefaultObject)
	static UObject* GetDefaultObjectForClass(TSubclassOf<UObject> type);
};