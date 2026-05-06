#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PerformAttack.generated.h"

/**
 * Behavior Tree Task: AI character attacks.
 * Returns Succeeded immediately - cooldown handled by BT decorator.
 */
UCLASS()
class NONEEDTOBRAIN_API UBTTask_PerformAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PerformAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};