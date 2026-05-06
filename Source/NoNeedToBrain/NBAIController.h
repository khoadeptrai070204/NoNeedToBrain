#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NBAIController.generated.h"

class UBehaviorTree;
class UAISenseConfig_Sight;

/**
 * AI Controller cho enemy character.
 *
 * Setup:
 * 1. Tao BP_AI_Controller_Base ke thua tu class nay.
 * 2. Trong BP, set BehaviorTree = BT_Enemy va Blackboard se tu detect tu BT.
 * 3. Assign BP_AI_Controller_Base vao BP_Hero_Big/Small lam AIControllerClass.
 *
 * Khi possess character, controller tu chay BehaviorTree va detect player qua sight sense.
 */
UCLASS()
class NONEEDTOBRAIN_API ANBAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANBAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

protected:
	/** Behavior Tree de chay khi possess pawn. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;

	/** Sight sense config - tu auto-create trong constructor. */
	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig = nullptr;

	/** Tam nhin AI - radius detect player. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float SightRadius = 1500.f;

	/** Mat dau (lose target) khi player ra ngoai radius nay. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float LoseSightRadius = 2000.f;

	/** Goc nhin (degrees, 0-180). 90 = 180 degree FOV. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float PeripheralVisionAngleDegrees = 90.f;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};