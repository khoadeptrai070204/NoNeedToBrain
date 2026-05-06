#include "BTTask_PerformGrab.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "NBCharacter.h"
#include "TimerManager.h"
#include "Engine/World.h"

UBTTask_PerformGrab::UBTTask_PerformGrab()
{
	NodeName = TEXT("Perform Grab");
}

EBTNodeResult::Type UBTTask_PerformGrab::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	ANBCharacter* Character = Cast<ANBCharacter>(AIController->GetPawn());
	if (!Character) return EBTNodeResult::Failed;

	Character->PerformAIGrab();

	// Wait for grab animation to finish.
	FTimerHandle Timer;
	UWorld* World = Character->GetWorld();
	if (World)
	{
		FTimerDelegate Delegate;
		TWeakObjectPtr<UBehaviorTreeComponent> WeakComp = &OwnerComp;
		Delegate.BindLambda([WeakComp]()
		{
			if (WeakComp.IsValid())
			{
				WeakComp->OnTaskFinished(nullptr, EBTNodeResult::Succeeded);
			}
		});
		World->GetTimerManager().SetTimer(Timer, Delegate, WaitForGrabDuration, false);
	}

	return EBTNodeResult::InProgress;
}
