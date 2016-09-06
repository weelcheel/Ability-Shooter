#include "AbilityShooter.h"
#include "Quest.h"
#include "UnrealNetwork.h"
#include "AbilityShooterPlayerController.h"

void FQuestObjective::IncrementCheckpoint()
{
	if (currentCheckpointCount + 1 < maxCheckpointCount)
		currentCheckpointCount++;
	else if (owner)
		owner->CurrentObjectiveCompleted();
}

UQuest::UQuest()
{
	key = "";
	title = FText::GetEmpty();
}

void UQuest::CurrentObjectiveCompleted()
{
	OnCurrentObjectiveCompleted();

	if (currentObjective + 1 < currentObjectives.Num())
		currentObjective++;
	else if (currentObjective + 1 >= currentObjectives.Num())
	{
		OnCompleted();
		if (IsValid(owningPlayer))
		{
			owningPlayer->CompleteQuest(this);
		}
	}
}

void UQuest::AddNewObjective(FQuestObjective& newObjective)
{
	newObjective.owner = this;
	currentObjectives.Add(newObjective);
}

FString UQuest::GetKey() const
{
	return key;
}

FText UQuest::GetTitle() const
{
	return title;
}

void UQuest::GetCurrentObjectives(TArray<FQuestObjective>& outArray) const
{
	outArray = currentObjectives;
}

int32 UQuest::GetCurrentObjective() const
{
	return currentObjective;
}

float UQuest::GetCurrentObjectiveCompletion() const
{
	if (currentObjective < currentObjectives.Num() && currentObjective >= 0)
		return currentObjectives[currentObjective].GetCurrentCheckpointProgress();
	else
		return 0.f;
}

void UQuest::IncrementCurrentObjectiveCheckpoint()
{
	if (currentObjective < currentObjectives.Num() && currentObjective >= 0)
		currentObjectives[currentObjective].IncrementCheckpoint();
}

void UQuest::SetPlayerOwner(AAbilityShooterPlayerController* newOwner)
{
	if (!IsValid(newOwner))
		return;

	owningPlayer = newOwner;
}

UQuest* UQuest::CreateQuest(TSubclassOf<UQuest> questClass)
{
	UQuest* newQuest = NewObject<UQuest>(GetTransientPackage(), questClass);

	return newQuest;
}

void UQuest::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(UQuest, key);
	DOREPLIFETIME(UQuest, title);
	DOREPLIFETIME(UQuest, currentObjectives);
	DOREPLIFETIME(UQuest, currentObjective);
}