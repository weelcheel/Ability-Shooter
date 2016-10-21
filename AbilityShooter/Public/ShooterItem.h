#pragma once

#include "ShooterItem.generated.h"

UCLASS(ABSTRACT, Blueprintable)
class AShooterItem : public AActor
{
	GENERATED_BODY()

protected:

	/* ui name of this item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	FText uiName;

	/* ui icon for this item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = UI)
	UTexture2D* uiIcon;

public:
	AShooterItem();

	/* static function for getting default objects in blueprints */
	UFUNCTION(BlueprintCallable, Category = DefaultObject)
	static UObject* GetDefaultObjectForClass(TSubclassOf<UObject> type);
};