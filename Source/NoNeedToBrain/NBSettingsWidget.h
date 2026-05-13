#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NBSettingsWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;
class UAudioComponent;

/** Input mode ma nguoi dung da chon. */
UENUM(BlueprintType)
enum class EInputMode : uint8
{
	MouseKeyboard	UMETA(DisplayName = "Mouse + Keyboard"),
	Gamepad			UMETA(DisplayName = "Gamepad"),
};

/** Delegate fire khi Settings widget dong (Confirm hoac Back).
 *  Menu widget subscribe vao day de biet khi nao restore UI. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsClosed);

UCLASS()
class NONEEDTOBRAIN_API UNBSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Mo widget: load saved settings, sync UI, add to viewport. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ShowSettings();

	/** Fire khi widget dong. Menu widget bind vao day de restore focus/visibility. */
	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnSettingsClosed OnClosed;

	// =========================================================
	// BindWidget — ten phai khop chinh xac trong BP Designer
	// =========================================================

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_MouseKeyboard;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Gamepad;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SLD_Volume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Volume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Confirm;

protected:
	virtual void NativeConstruct() override;

	EInputMode PendingInputMode = EInputMode::MouseKeyboard;
	float PendingVolume = 1.f;
	EInputMode SavedInputMode = EInputMode::MouseKeyboard;
	float SavedVolume = 1.f;

	void RefreshInputButtons();
	void ApplyVolume(float Volume);
	void ApplyInputMode(EInputMode Mode);
	void LoadSavedSettings();
	void SaveSettings();

	/** Dong widget + fire OnClosed delegate. */
	void CloseSettings();

	UFUNCTION() void OnMouseKeyboardClicked();
	UFUNCTION() void OnGamepadClicked();
	UFUNCTION() void OnVolumeChanged(float Value);
	UFUNCTION() void OnConfirmClicked();
};