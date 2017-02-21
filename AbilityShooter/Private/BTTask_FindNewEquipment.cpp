#include "AbilityShooter.h"
#include "BTTask_FindNewEquipment.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "AbilityAIController.h"
#include "AbilityShooterBot.h"
#include "EquipmentItem.h"
#include "AbilityShooterGameMode.h"

UBTTask_FindNewEquipment::UBTTask_FindNewEquipment()
{
	NodeName = "Find Nearest Unowned Equipment";
}

EBTNodeResult::Type UBTTask_FindNewEquipment::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAbilityAIController* MyController = Cast<AAbilityAIController>(OwnerComp.GetAIOwner());
	AAbilityShooterBot* MyBot = MyController ? Cast<AAbilityShooterBot>(MyController->GetPawn()) : NULL;
	if (MyBot == NULL)
	{
		return EBTNodeResult::Failed;
	}

	AAbilityShooterGameMode* GameMode = MyBot->GetWorld()->GetAuthGameMode<AAbilityShooterGameMode>();
	if (GameMode == NULL)
	{
		return EBTNodeResult::Failed;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	AEquipmentItem* BestPickup = NULL;
	float BestDistSq = MAX_FLT;

	for (TActorIterator<AEquipmentItem> itr(GameMode->GetWorld()); itr; ++itr)
	{
		AEquipmentItem* AmmoPickup = (*itr);
		if (AmmoPickup && AmmoPickup->IsA(MyBot->GetCurrentEquipment()->StaticClass()) && AmmoPickup->GetCurrentState() == EEquipmentState::NoOwner)
		{
			const float DistSq = (AmmoPickup->GetActorLocation() - MyLoc).SizeSquared();
			if (BestDistSq == -1 || DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPickup = AmmoPickup;
			}
		}
	}

	if (BestPickup)
	{
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), BestPickup->GetActorLocation());
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}