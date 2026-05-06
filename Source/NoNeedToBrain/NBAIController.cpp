#include "NBAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NBCharacter.h"

ANBAIController::ANBAIController()
{
	// Tao Perception Component.
	UAIPerceptionComponent* PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));
	SetPerceptionComponent(*PerceptionComp);

	// Tao Sight Config.
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->SetMaxAge(5.f);

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	// Bind delegate ngay trong constructor de chac chan khong miss event.
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ANBAIController::OnTargetPerceptionUpdated);
}

void ANBAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Run Behavior Tree.
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void ANBAIController::OnUnPossess()
{
	Super::OnUnPossess();
}

void ANBAIController::BeginPlay()
{
	Super::BeginPlay();

	// FORCE set TargetActor = Player Pawn after delay (to ensure player has spawned).
	// Su dung weak pointer de tranh crash neu AI Controller bi destroy truoc khi timer fire.
	TWeakObjectPtr<ANBAIController> WeakSelf = this;

	FTimerHandle DelayHandle;
	GetWorldTimerManager().SetTimer(DelayHandle, [WeakSelf]()
		{
			// Verify AI controller van valid.
			if (!WeakSelf.IsValid()) return;

			ANBAIController* Self = WeakSelf.Get();
			if (!Self) return;

			UBlackboardComponent* BB = Self->GetBlackboardComponent();
			if (!BB) return;

			UWorld* World = Self->GetWorld();
			if (!World) return;

			APlayerController* PC = World->GetFirstPlayerController();
			if (!PC) return;

			APawn* PlayerPawn = PC->GetPawn();
			if (!PlayerPawn) return;

			BB->SetValueAsObject(TEXT("TargetActor"), PlayerPawn);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
					FString::Printf(TEXT("[AI] Target SET: %s"), *PlayerPawn->GetName()));
			}
		}, 0.5f, false);
}

void ANBAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !GetBlackboardComponent()) return;

	UBlackboardComponent* BB = GetBlackboardComponent();

	// Chi care neu Actor la NBCharacter va KHONG phai chinh AI nay (de tranh self-detect).
	ANBCharacter* AsCharacter = Cast<ANBCharacter>(Actor);
	if (!AsCharacter) return;
	if (AsCharacter == GetPawn()) return;

	// Set/Clear TargetActor theo trang thai sense.
	if (Stimulus.WasSuccessfullySensed())
	{
		BB->SetValueAsObject(TEXT("TargetActor"), Actor);
	}
	else
	{
		UObject* CurrentTarget = BB->GetValueAsObject(TEXT("TargetActor"));
		if (CurrentTarget == Actor)
		{
			BB->ClearValue(TEXT("TargetActor"));
		}
	}
}