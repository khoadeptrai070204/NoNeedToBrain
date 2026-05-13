#include "NBSettingsWidget.h"
#include "NBGameInstance.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "AudioDevice.h"
#include "GameFramework/PlayerController.h"

// =========================================================
// Lifecycle
// =========================================================

void UNBSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BTN_MouseKeyboard)
		BTN_MouseKeyboard->OnClicked.AddUniqueDynamic(this, &UNBSettingsWidget::OnMouseKeyboardClicked);

	if (BTN_Gamepad)
		BTN_Gamepad->OnClicked.AddUniqueDynamic(this, &UNBSettingsWidget::OnGamepadClicked);

	if (BTN_Confirm)
		BTN_Confirm->OnClicked.AddUniqueDynamic(this, &UNBSettingsWidget::OnConfirmClicked);

	if (SLD_Volume)
	{
		SLD_Volume->OnValueChanged.AddUniqueDynamic(this, &UNBSettingsWidget::OnVolumeChanged);
		SLD_Volume->SetMinValue(0.f);
		SLD_Volume->SetMaxValue(1.f);
		SLD_Volume->SetStepSize(0.05f);
	}
}

void UNBSettingsWidget::ShowSettings()
{
	LoadSavedSettings();

	PendingInputMode = SavedInputMode;
	PendingVolume = SavedVolume;

	if (SLD_Volume)
		SLD_Volume->SetValue(PendingVolume);

	// Sync label % va button state.
	OnVolumeChanged(PendingVolume);
	RefreshInputButtons();

	if (!IsInViewport())
		AddToViewport(50);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		FInputModeUIOnly UIMode;
		UIMode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(UIMode);
	}
}

// =========================================================
// Helpers
// =========================================================

void UNBSettingsWidget::RefreshInputButtons()
{
	// Button dang active -> disabled (visual feedback nguoi dung biet mode nao dang chon).
	// Button con lai -> enabled.
	if (BTN_MouseKeyboard)
		BTN_MouseKeyboard->SetIsEnabled(PendingInputMode != EInputMode::MouseKeyboard);

	if (BTN_Gamepad)
		BTN_Gamepad->SetIsEnabled(PendingInputMode != EInputMode::Gamepad);
}

void UNBSettingsWidget::ApplyVolume(float Volume)
{
	// SetTransientMasterVolume anh huong tat ca SFX + Music trong World.
	if (UWorld* World = GetWorld())
	{
		if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
		{
			AudioDevice->SetTransientPrimaryVolume(Volume);
		}
	}

	// Dong thoi update music component trong GameInstance de nhat quan.
	if (UNBGameInstance* GI = GetGameInstance<UNBGameInstance>())
	{
		if (GI->CurrentMusicComp && GI->CurrentMusicComp->IsPlaying())
		{
			GI->CurrentMusicComp->SetVolumeMultiplier(Volume);
		}
	}
}

void UNBSettingsWidget::ApplyInputMode(EInputMode Mode)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	if (Mode == EInputMode::Gamepad)
	{
		// Gamepad: an cursor, UI navigate bang analog stick / DPad.
		PC->bShowMouseCursor = false;
		FInputModeUIOnly UIMode;
		UIMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(UIMode);
	}
	else
	{
		// Mouse + Keyboard: hien cursor.
		PC->bShowMouseCursor = true;
		FInputModeUIOnly UIMode;
		PC->SetInputMode(UIMode);
	}
}

void UNBSettingsWidget::LoadSavedSettings()
{
	if (UNBGameInstance* GI = GetGameInstance<UNBGameInstance>())
	{
		SavedInputMode = (GI->SavedInputModeValue == 1)
			? EInputMode::Gamepad
			: EInputMode::MouseKeyboard;
		SavedVolume = GI->SavedVolume;
	}
}

void UNBSettingsWidget::SaveSettings()
{
	if (UNBGameInstance* GI = GetGameInstance<UNBGameInstance>())
	{
		GI->SavedInputModeValue = (PendingInputMode == EInputMode::Gamepad) ? 1 : 0;
		GI->SavedVolume = PendingVolume;
	}
}

// =========================================================
// Callbacks
// =========================================================

void UNBSettingsWidget::OnMouseKeyboardClicked()
{
	PendingInputMode = EInputMode::MouseKeyboard;
	RefreshInputButtons();
}

void UNBSettingsWidget::OnGamepadClicked()
{
	PendingInputMode = EInputMode::Gamepad;
	RefreshInputButtons();
}

void UNBSettingsWidget::OnVolumeChanged(float Value)
{
	PendingVolume = Value;

	// Update label realtime.
	if (TXT_Volume)
	{
		const int32 Percent = FMath::RoundToInt(Value * 100.f);
		TXT_Volume->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Percent)));
	}

	// Preview volume realtime khi keo slider.
	ApplyVolume(Value);
}

void UNBSettingsWidget::OnConfirmClicked()
{
	const bool bInputChanged = (PendingInputMode != SavedInputMode);
	const bool bVolumeChanged = !FMath::IsNearlyEqual(PendingVolume, SavedVolume, 0.01f);

	if (bInputChanged || bVolumeChanged)
	{
		SaveSettings();
		UE_LOG(LogTemp, Log, TEXT("[Settings] Saved: InputMode=%d, Volume=%.2f"),
			(int32)PendingInputMode, PendingVolume);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Settings] No changes, closing."));
	}

	CloseSettings();
}

void UNBSettingsWidget::CloseSettings()
{
	// Remove widget khoi viewport.
	RemoveFromParent();

	// Restore input mode ve UIOnly + show cursor de menu hoat dong binh thuong.
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		FInputModeUIOnly UIMode;
		PC->SetInputMode(UIMode);
	}

	// Fire delegate de menu widget biet Settings da dong.
	// Menu widget (WBP_MainMenu) can bind vao day de:
	// - Set visibility cua chinh no ve Visible neu bi an.
	// - Re-focus vao chinh no.
	OnClosed.Broadcast();
}