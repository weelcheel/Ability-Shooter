#pragma once

#include "StatsManager.generated.h"

UENUM(BlueprintType)
enum class EStat : uint8
{
	ES_Atk 	UMETA(DisplayName = "Attack"),
	ES_Def 	UMETA(DisplayName = "Defense"),
	ES_SpAtk UMETA(DisplayName = "Special Attack"),
	ES_SpDef UMETA(DisplayName = "Special Defense"),
	ES_EquipUseRate UMETA(DisplayName = "Equipment Use Rate"),
	ES_Move UMETA(DisplayName = "Movement Speed"),
	ES_CDR UMETA(DisplayName = "Cooldown Reduction"),
	ES_HP UMETA(DisplayName = "Health"),
	ES_CritChance UMETA(DisplayName = "Critical Hit Chance"),
	ES_SpecialCritChance UMETA(DisplayName = "Special Damage Critical Hit Chance"),
	ES_CritRatio UMETA(DisplayName = "Critical Hit Percent Damage"),
	ES_Acc UMETA(DisplayName = "Accuracy"),
	ES_Eva UMETA(DisplayName = "Evasiveness"),
	ES_HPDrain UMETA(DisplayName = "Health Drain"),
	ES_Max UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FBaseStats
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float attack = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float defense = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float specialAttack = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float specialDefense = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float equipUseRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float movementSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float cooldownReduction = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float HP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float critChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float specialCritChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float critRatio = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float accuracy = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float evasiveness = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat)
	float healthDrain = 0.f;
};

UCLASS()
class UStatsManager : public UObject
{
	friend class AAbilityShooterCharacter;

	GENERATED_BODY()

protected:

	/* owning character for this manager */
	AAbilityShooterCharacter* characterOwner;

public:

	UStatsManager();

	/* sets the new owning character */
	void SetOwningCharacter(AAbilityShooterCharacter* newOwner);

	/* gets the current value for the specified stat */
	UFUNCTION(BlueprintCallable, Category = Stats)
	float GetCurrentStatValue(EStat stat);

	/* gets the current value of a stat not factoring in any effects and everything at max value */
	UFUNCTION(BlueprintCallable, Category = Stats)
	float GetBaseStatValue(EStat stat);
};