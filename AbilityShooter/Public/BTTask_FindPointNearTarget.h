#pragma once
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindPointNearTarget.generated.h"

// Bot AI task that tries to find a location near the current enemy
UCLASS()
class UBTTask_FindPointNearTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
public:
	UBTTask_FindPointNearTarget();
};