#pragma once

#include "ShooterDamage.generated.h"

USTRUCT(BlueprintType)
struct FShooterDamage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	float Damage; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	FDamageEvent DamageEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	AController* EventInstigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	AActor* DamageCauser;
};