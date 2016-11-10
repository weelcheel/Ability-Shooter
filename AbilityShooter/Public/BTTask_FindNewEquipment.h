#pragma once
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindNewEquipment.generated.h"

// Bot AI Task that attempts to locate a pickup 
UCLASS()
class UBTTask_FindNewEquipment : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	UBTTask_FindNewEquipment();
};