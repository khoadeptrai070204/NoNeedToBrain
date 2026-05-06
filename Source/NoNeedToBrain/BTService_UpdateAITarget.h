#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateAITarget.generated.h"

/**
 * BT Service: chay moi tick, update blackboard:
 * - DistanceToTarget (float): khoang cach toi target.
 * - bIsTargetInAttackRange (bool): true neu trong tam tan cong.
 *
 * Cung face AI ve phia target neu co target.
 */
UCLASS()
class NONEEDTOBRAIN_API UBTService_UpdateAITarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateAITarget();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** Range tan cong - khi distance < range thi set bIsTargetInAttackRange = true. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackRange = 150.f;

	/** Co face AI ve phia target moi tick khong. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bFaceTargetEachTick = true;
};
