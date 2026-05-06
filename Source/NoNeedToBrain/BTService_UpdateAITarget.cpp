#include "BTService_UpdateAITarget.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NBCharacter.h"

UBTService_UpdateAITarget::UBTService_UpdateAITarget()
{
	NodeName = TEXT("Update AI Target Info");
	bNotifyTick = true;
	Interval = 0.1f;  // tick moi 0.1s, khong can moi frame
	RandomDeviation = 0.f;
}

void UBTService_UpdateAITarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	ANBCharacter* MyChar = Cast<ANBCharacter>(AIController->GetPawn());
	if (!MyChar) return;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));

	if (!Target)
	{
		BB->SetValueAsFloat(TEXT("DistanceToTarget"), -1.f);
		BB->SetValueAsBool(TEXT("bIsTargetInAttackRange"), false);
		return;
	}

	const float Distance = FVector::Dist(MyChar->GetActorLocation(), Target->GetActorLocation());
	const bool bInRange = Distance <= AttackRange;

	BB->SetValueAsFloat(TEXT("DistanceToTarget"), Distance);
	BB->SetValueAsBool(TEXT("bIsTargetInAttackRange"), bInRange);
	BB->SetValueAsVector(TEXT("TargetLocation"), Target->GetActorLocation());

	// AUTO SPRINT: khi xa target (>500 unit) thi sprint.
	const bool bShouldSprint = (Distance > 500.f);
	MyChar->SetAISprinting(bShouldSprint);

	if (bFaceTargetEachTick)
	{
		MyChar->AIFaceTarget(Target);
	}
}