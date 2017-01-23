#pragma once

#include "ShooterDamage.h"
#include "ShooterItem.generated.h"

class AAbilityShooterCharacter;

UCLASS(ABSTRACT, Blueprintable)
class AShooterItem : public AActor
{
	GENERATED_BODY()

protected:

	/* ===============================Event Handlers=========================================*/

	/* whenever the owner takes damage */
	UFUNCTION(BlueprintImplementableEvent, Category = Damage)
	void OnOwnerDamaged(FShooterDamage damage, float dmgIn, float& dmgOut);

	/* whenever the owner deals damage */
	UFUNCTION(BlueprintImplementableEvent, Category = Damage)
	void OnOwnerDealtDamage(FShooterDamage damage, AAbilityShooterCharacter* damagedCharacter, float dmgIn, float& dmgOut);

	/* whenever the owner collides with something (useful for dashes) */
	UFUNCTION(BlueprintImplementableEvent, Category = Collision)
	void OnOwnerCollided(UPrimitiveComponent* hitComp, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit);

	/* whenever the owner starts dashing */
	UFUNCTION(BlueprintImplementableEvent, Category = Dash)
	void OnOwnerDashStart(FVector dashLocation, float dashSpeed);

	/* whenever the owner stops dashing */
	UFUNCTION(BlueprintImplementableEvent, Category = Dash)
	void OnOwnerDashEnd();

public:
	AShooterItem();

	/* ui name of this item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	FText uiName;

	/* description of the item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	FText uiDescription;

	/* ui icon for this item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	UTexture2D* uiIcon;

	/* static function for getting default objects in blueprints */
	UFUNCTION(BlueprintCallable, Category = DefaultObject)
	static UObject* GetDefaultObjectForClass(TSubclassOf<UObject> type);
};