#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PerformGrab.generated.h"

/**
 * Behavior Tree Task: AI character grabs target.
 * Returns InProgress while grab animation plays, Succeeded when done.
 */
UCLASS()
class NONEEDTOBRAIN_API UBTTask_PerformGrab : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PerformGrab();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Grab")
	float WaitForGrabDuration = 1.0f;
};
