#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NBGameMode.generated.h"

class ANBCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerWinSignature, AActor*, Winner);

/**
 * Game Mode chinh cho NoNeedToBrain.
 * Win condition: chi con 1 player con song (HP > 0).
 * Hero spawn: dua vao SelectedHeroClass cua tung Player Controller.
 */
UCLASS()
class NONEEDTOBRAIN_API ANBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANBGameMode();

	/** Override de spawn pawn theo hero class cua tung controller. */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	/** Override de doc URL options "hero" khi client connect. */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Override de set SelectedHeroClass NGAY khi player connect, truoc khi spawn pawn. */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

	/** Goi tu NBCharacter khi character chet. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void OnCharacterDied(ANBCharacter* DeadCharacter);

	/** Broadcast khi co nguoi thang. */
	UPROPERTY(BlueprintAssignable, Category = "Game")
	FOnPlayerWinSignature OnPlayerWin;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bGameEnded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	AActor* WinnerActor = nullptr;

	/** Default fallback heroes. Set trong BP. */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	TSubclassOf<ANBCharacter> DefaultBigClass;

	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	TSubclassOf<ANBCharacter> DefaultSmallClass;

protected:
	virtual void BeginPlay() override;

	void CheckWinCondition();
};