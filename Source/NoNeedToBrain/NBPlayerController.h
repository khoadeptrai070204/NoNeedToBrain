#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NBPlayerController.generated.h"

class ANBCharacter;
class UNBEndScreenWidget;

UCLASS()
class NONEEDTOBRAIN_API ANBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANBPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Khi possess pawn: apply input mode tu Settings (Mouse+KB vs Gamepad). */
	virtual void OnPossess(APawn* InPawn) override;

	/** Hero class duoc chon - replicated tu client len server. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hero")
	TSubclassOf<ANBCharacter> SelectedHeroClass;

	/** Client goi server de set hero (khi confirm tu menu). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Hero")
	void Server_SetSelectedHero(TSubclassOf<ANBCharacter> InHeroClass);

	UFUNCTION(BlueprintPure, Category = "Hero")
	TSubclassOf<ANBCharacter> GetSelectedHeroClass() const { return SelectedHeroClass; }

	// =========================================================
	// End Screen (Victory / Defeated)
	// =========================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UNBEndScreenWidget> EndScreenClass;

	UFUNCTION(Client, Reliable)
	void Client_ShowEndScreen(bool bIsVictory);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowEndScreen(bool bIsVictory);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UNBEndScreenWidget> EndScreenInstance;
};