#pragma once

#include "StatsManager.h"
#include "Effect.generated.h"

class AAbilityShooterCharacter;

/* what stat to alter and how much to alter it by (adds the amount to the stat) */
USTRUCT(Blueprintable)
struct FEffectStatAlter
{
	GENERATED_USTRUCT_BODY()

	/* stat to alter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectStat)
	EStat alteredStat;

	/* how much to alter the stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectStat)
	float deltaStat;

	/* how much to alter the stat (unaffected by stacks) */
	float baseDeltaStat;

	/* whether or not this stat alter is multiplied by effect stacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectStat)
	bool bShouldMultiplyWithStacks;
};

UCLASS(Blueprintable)
class UEffect : public UObject
{
	friend class AAbilityShooterCharacter;
	friend class UStatsManager;

	GENERATED_BODY()

protected:

	/* name of the effect for in-game ui */
	UPROPERTY()
	FString key;

	/* name of the effect for in-game ui */
	UPROPERTY()
	FText uiName;

	/* description of the effect for in-game ui */
	UPROPERTY()
	FText description;

	/* get the total time before this effect expires; does not change from initial set unless the effect is reset to a different length */
	UPROPERTY()
	float duration;

	/* number of stacks that this effect has (if it counts stacks) */
	int32 stacks;

	/* whether or not this effect persists through death */
	bool bPersistThruDeath;

	/* whether or not this effect has stacks */
	bool bHasStacks;

	/* stats this effect alters */
	UPROPERTY()
	TArray<FEffectStatAlter> statAlters;

	/* timer that expires whenever this effect is finished */
	FTimerHandle expirationTimer;

	/* whether or not this effect has been initialized */
	bool bEffectInitialized = false;

	/* character to apply this to */
	UPROPERTY(BlueprintReadWrite, Category = EffectInfo)
	AAbilityShooterCharacter* appliedCharacter;

	/* called whenever this effect's expiration timer is called */
	void OnEffectExpired();

	/* blueprint hook for special effects that need to perform logic on application */
	UFUNCTION(BlueprintImplementableEvent, Category = Effect)
	void OnEffectAppliedToCharacter(AAbilityShooterCharacter* appCharacter);

	/* blueprint hook for special effects that need to perform logic on expiration or removal */
	UFUNCTION(BlueprintImplementableEvent, Category = Effect)
	void OnEffectRemovedFromCharacter(AAbilityShooterCharacter* removedCharacter);

public:
	UEffect();

	//--- getter functions ---/

	/* ui name*/
	UFUNCTION(BlueprintCallable, Category = Effect)
	FText GetUIName() const;

	/* ui description*/
	UFUNCTION(BlueprintCallable, Category = Effect)
	FText GetDescription() const;

	/* effect duration */
	UFUNCTION(BlueprintCallable, Category = Effect)
	float GetDuration() const;

	/* effect timer */
	UFUNCTION(BlueprintCallable, Category = Effect)
	void GetExpirationTimer(FTimerHandle& inHandle) const;

	/* is effect initialized */
	UFUNCTION(BlueprintCallable, Category = Effect)
	bool IsInitialized() const;

	/* get the effect key */
	UFUNCTION(BlueprintCallable, Category = Effect)
	FString GetKey() const;

	/* gets whether or not this effect stacks*/
	UFUNCTION(BlueprintCallable, Category = Effect)
	bool DoesStack() const;

	/* get the number of stacks this effect has */
	UFUNCTION(BlueprintCallable, Category = Effect)
	int32 GetStacks() const;

	/* add a number of stacks */
	UFUNCTION(BlueprintCallable, Category = Effect)
	void AddStacks(int32 deltaAmt);

	/* add a number of stacks */
	UFUNCTION(BlueprintCallable, Category = Effect)
	void SetStacks(int32 newAmt);

	//--- ---

	/* called to initialize this effect from init info */
	void Initialize(const struct FEffectInitInfo& initInfo, AAbilityShooterCharacter* characterOwner);

	/* sets the expiration timer */
	void SetExpirationTimer();
};

USTRUCT(Blueprintable)
struct FEffectInitInfo
{
	GENERATED_USTRUCT_BODY()

	/* name of the effect for in-game ui */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	FText uiName;

	/* description of the effect for in-game ui */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	FText description;

	/* get the total time before this effect expires; does not change from initial set unless the effect is reset to a different length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	float duration;

	/* list of stat alterations that this effect performs on application */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	TArray<FEffectStatAlter> statAlters;

	/* whether or not this effect keeps going thru death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	bool bDoesPersistThruDeath;

	/* whether or not this effect keeps track of stack counts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	bool bShouldStack;

	/* number of stacks this effect starts with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	int32 startStacks;

	/* what class of effect to apply */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	TSubclassOf<UEffect> effectType;

	/* timer handle for if this effect is persisting through death */
	FTimerHandle persistentTimer;

	/* key for if this effect is persisting through death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EffectInfo)
	FString persistentKey;

	/* game time this persistent effect was removed from the character */
	float persistentTime;

	FEffectInitInfo()
	{
		uiName = FText::GetEmpty();
		description = FText::GetEmpty();
		persistentKey = "";
		duration = -1.f;
		bDoesPersistThruDeath = false;
		bShouldStack = false;
		effectType = UEffect::StaticClass();
		persistentTime = -1.f;
		startStacks = 0;
	}
};