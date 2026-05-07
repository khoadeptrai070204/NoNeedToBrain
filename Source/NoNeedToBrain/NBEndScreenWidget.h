#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NBEndScreenWidget.generated.h"

class UImage;
class UButton;
class UTexture2D;

/**
 * Widget hien thi ket qua Victory / Defeated cuoi tran.
 * - IMG_Result: Image se duoc swap brush theo bIsVictory.
 * - BTN_BackToMenu: button quay ve main menu.
 * - T_Victory / T_Defeated: 2 texture set trong BP child.
 *
 * Su dung: Server quyet dinh ai win/lose, goi Client_ShowEndScreen tren NBPlayerController,
 * PC tao widget nay va goi ShowResult(bIsVictory).
 */
UCLASS()
class NONEEDTOBRAIN_API UNBEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Goi tu PlayerController de set Image va hien thi. */
	UFUNCTION(BlueprintCallable, Category = "EndScreen")
	void ShowResult(bool bIsVictory);

	/** Texture cho Victory. Set trong BP child (BP_NBEndScreenWidget) Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EndScreen")
	TObjectPtr<UTexture2D> T_Victory;

	/** Texture cho Defeated. Set trong BP child Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EndScreen")
	TObjectPtr<UTexture2D> T_Defeated;

	/** Ten map main menu de open khi bam Back. Default: "MainMenu". */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EndScreen")
	FName MainMenuMapName = TEXT("MainMenu");

protected:
	virtual void NativeConstruct() override;

	/** BindWidget: phai co widget cung ten "IMG_Result" trong BP designer. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IMG_Result;

	/** BindWidget: phai co widget cung ten "BTN_BackToMenu" trong BP designer. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_BackToMenu;

	/** Handler cho button Back. */
	UFUNCTION()
	void OnBackToMenuClicked();
};
