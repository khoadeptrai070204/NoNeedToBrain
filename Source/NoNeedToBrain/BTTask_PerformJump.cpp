#include "BTTask_PerformJump.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "NBCharacter.h"

UBTTask_PerformJump::UBTTask_PerformJump()
{
	NodeName = TEXT("Perform Jump");
}

EBTNodeResult::Type UBTTask_PerformJump::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	ANBCharacter* Character = Cast<ANBCharacter>(AIController->GetPawn());
	if (!Character) return EBTNodeResult::Failed;

	Character->PerformAIJump();

	return EBTNodeResult::Succeeded;
}
