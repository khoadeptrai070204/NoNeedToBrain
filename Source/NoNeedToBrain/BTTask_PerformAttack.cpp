#include "BTTask_PerformAttack.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "NBCharacter.h"

UBTTask_PerformAttack::UBTTask_PerformAttack()
{
	NodeName = TEXT("Perform Attack");
}

EBTNodeResult::Type UBTTask_PerformAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	ANBCharacter* Character = Cast<ANBCharacter>(AIController->GetPawn());
	if (!Character) return EBTNodeResult::Failed;

	// Goi attack qua public AI function.
	Character->PerformAIAttack();

	// Tra Succeeded ngay - khong can latent.
	// Cooldown decorator trong BT se handle "wait between attacks".
	return EBTNodeResult::Succeeded;
}