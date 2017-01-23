#pragma once

#include "Engine/EngineTypes.h"
#include "GameFramework/DamageType.h"
#include "ShooterDamage.generated.h"

USTRUCT(BlueprintType)
struct FShooterUIDamage
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ShooterDamage)
		FString damagingShooterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ShooterDamage)
		FText damagingEntityName;

	FShooterUIDamage()
	{

	}
};

USTRUCT(BlueprintType)
struct FShooterDamage : public FPointDamageEvent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	bool bIsHeadshot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	AController* EventInstigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	AActor* DamageCauser;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	FShooterUIDamage DamageCauserInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	FHitResult PublicHitInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	float PublicDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShooterDamage)
	TSubclassOf<UDamageType> PublicDamageType;
};

UCLASS()
class UPhysicalDamage : public UDamageType
{
	GENERATED_BODY()

public:
	UPhysicalDamage()
	{

	}
};

UCLASS()
class USpecialDamage : public UDamageType
{
	GENERATED_BODY()

public:
	USpecialDamage()
	{

	}
};