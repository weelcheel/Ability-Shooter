#pragma once

#include "Quest.generated.h"

class UQuest;

USTRUCT(BlueprintType)
struct FQuestReward
{
	GENERATED_USTRUCT_BODY()

	/* ui text name for the objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestReward)
	FText title;

	/* ui picture to show the user */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestReward)
	UTexture* icon;

	/* ui color to display the text in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestReward)
	FColor titleColor;
};

USTRUCT(BlueprintType)
struct FQuestObjective
{
	GENERATED_USTRUCT_BODY()

	/* key symbol for this objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestObjective)
	FString key;

	/* ui text name for the objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestObjective)
	FText title;

	/* max number of checkpoints for this objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestObjective)
	int32 maxCheckpointCount;

	/* current number of checkpoints for this objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestObjective)
	int32 currentCheckpointCount;

	/* array of locations that represent waypoints to show the Shooter when the objective is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuestObjective)
	TArray<FVector> waypoints;

	/* quest owner of this objective */
	UQuest* owner;

	FQuestObjective()
	{
		key = "";
		title = FText::GetEmpty();
		maxCheckpointCount = 0;
		currentCheckpointCount = 0;
		owner = nullptr;
	}

	/* gets the current progress percentage of completion for this quest objective */
	float GetCurrentCheckpointProgress() const
	{
		if (maxCheckpointCount > 0)
			return (float)currentCheckpointCount / (float)maxCheckpointCount;
		else
			return 0.f;
	}

	/* increments the current checkpoint and finishes the objective if needed */
	void IncrementCheckpoint();
};

UCLASS(ABSTRACT, Blueprintable)
class UQuest : public UObject
{
	friend class AAbilityShooterPlayerController;

	GENERATED_BODY()

protected:
	/* key symbol for the quest */
	UPROPERTY(EditDefaultsOnly, Category=Quest, replicated)
	FString key;

	/* ui text name for the quest */
	UPROPERTY(EditDefaultsOnly, Category = Quest, replicated)
	FText title;

	/* array of objectives for this quest */
	UPROPERTY(EditDefaultsOnly, Category = Quest, replicated)
	TArray<FQuestObjective> currentObjectives;

	/* index of the current objective this quest is on. all objectives before this index are considered completed. */
	UPROPERTY(EditDefaultsOnly, Category = Quest, replicated)
	int32 currentObjective;

	/* expected rewards */
	UPROPERTY(EditDefaultsOnly, Category = Quest, replicated)
	TArray<FQuestReward> rewards;
	
	/* player controller that owns this quest */
	UPROPERTY(BlueprintReadOnly, Category=Quest)
	AAbilityShooterPlayerController* owningPlayer;

	/* blueprint hook for on objective completed logic */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest)
	void OnCurrentObjectiveCompleted();

	/* blueprint hook for on quest completed logic */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest)
	void OnCompleted();

public:
	UQuest();

	/* checks to see if the quest is completed */
	void CurrentObjectiveCompleted();

	/* adds an objective to the current objectives */
	UFUNCTION(BlueprintCallable, Category = Quest)
	void AddNewObjective(UPARAM(ref) FQuestObjective& newObjective);

	/* gets the key for the quest */
	FString GetKey() const;

	/* gets the title of the quest */
	UFUNCTION(BlueprintCallable, Category = Quest)
	FText GetTitle() const;

	/* gets the objectives for this quest */
	UFUNCTION(BlueprintCallable, Category = Quest)
	void GetCurrentObjectives(TArray<FQuestObjective>& outArray) const;

	/* gets the index for the current objective */
	UFUNCTION(BlueprintCallable, Category = Quest)
	int32 GetCurrentObjective() const;

	/* get the current objective's progress percentage */
	UFUNCTION(BlueprintCallable, Category = Quest)
	float GetCurrentObjectiveCompletion() const;

	/* increment the current objective's checkpoint */
	UFUNCTION(BlueprintCallable, Category = Quest)
	void IncrementCurrentObjectiveCheckpoint();

	/* sets the current player owner */
	void SetPlayerOwner(AAbilityShooterPlayerController* newOwner);

	/* networking support for uobject */
	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}

	/* static function to spawn a quest of a given class. quests can be derived from in blueprints to allow for quests to be created like assets */
	UFUNCTION(BlueprintCallable, Category = Quest)
	static UQuest* CreateQuest(TSubclassOf<UQuest> questClass);
};