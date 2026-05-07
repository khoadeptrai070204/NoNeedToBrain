#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NBPlayerController.generated.h"

class ANBCharacter;
class UNBEndScreenWidget;

/**
 * Player Controller chinh cho NoNeedToBrain.
 * Luu hero class duoc chon, replicate len server de spawn dung pawn.
 */
UCLASS()
class NONEEDTOBRAIN_API ANBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANBPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Hero class duoc chon - replicated tu client len server. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hero")
	TSubclassOf<ANBCharacter> SelectedHeroClass;

	/** Client goi server de set hero (khi confirm tu menu). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Hero")
	void Server_SetSelectedHero(TSubclassOf<ANBCharacter> InHeroClass);

	/** BlueprintCallable de UI co the read. */
	UFUNCTION(BlueprintPure, Category = "Hero")
	TSubclassOf<ANBCharacter> GetSelectedHeroClass() const { return SelectedHeroClass; }

	// =========================================================
	// End Screen (Victory / Defeated)
	// =========================================================

	/** Class cua End Screen widget. Set BP_NBEndScreenWidget trong BP_NBPlayerController defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UNBEndScreenWidget> EndScreenClass;

	/** Server goi xuong client de hien End Screen. bIsVictory=true neu client nay thang. */
	UFUNCTION(Client, Reliable)
	void Client_ShowEndScreen(bool bIsVictory);

	/** Goi truc tiep tren client (vi du khi test offline) - tao widget va show. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowEndScreen(bool bIsVictory);

protected:
	/** Instance cua End Screen widget hien tai (trong client). */
	UPROPERTY(Transient)
	TObjectPtr<UNBEndScreenWidget> EndScreenInstance;
};