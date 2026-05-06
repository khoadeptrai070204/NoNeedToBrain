#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PerformJump.generated.h"

/**
 * Behavior Tree Task: AI character jumps.
 * Use under decorator with random chance for emergent behavior.
 */
UCLASS()
class NONEEDTOBRAIN_API UBTTask_PerformJump : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PerformJump();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
