#pragma once

#include "LatentActions.h"

class FCharacterAction : public FPendingLatentAction
{
public:
	float timeRemaining;
	FName executionFunction;
	int32 outputLink;
	FWeakObjectPtr callbackTarget;
	bool bShouldEndPrematurely;

	FCharacterAction(float duration, const FLatentActionInfo& latentInfo)
		: timeRemaining(duration)
		, executionFunction(latentInfo.ExecutionFunction)
		, outputLink(latentInfo.Linkage)
		, callbackTarget(latentInfo.CallbackTarget)
		, bShouldEndPrematurely(false)
	{

	}

	FCharacterAction()
	{
		timeRemaining = -1.f;
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		timeRemaining -= Response.ElapsedTime();
		Response.FinishAndTriggerIf(timeRemaining <= 0.f || bShouldEndPrematurely, executionFunction, outputLink, callbackTarget);
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return FString::Printf(*NSLOCTEXT("CharacterAction", "CharacterActionTime", "Character Action (%.3f seconds left)").ToString(), timeRemaining);
	}
#endif
};