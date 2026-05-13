#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NBGameInstance.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * Game Instance giu music + settings persist qua cac map.
 */
UCLASS()
class NONEEDTOBRAIN_API UNBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// =========================================================
	// Music
	// =========================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TObjectPtr<USoundBase> MenuMusic = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TObjectPtr<USoundBase> MatchMusic = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MusicVolume = 0.7f;

	UFUNCTION(BlueprintCallable, Category = "Music")
	void PlayMenuMusic();

	UFUNCTION(BlueprintCallable, Category = "Music")
	void PlayMatchMusic();

	UFUNCTION(BlueprintCallable, Category = "Music")
	void StopMusic();

	/** Public de NBSettingsWidget chinh volume realtime. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentMusicComp = nullptr;

	// =========================================================
	// Settings persistence
	// =========================================================

	/** Input mode nguoi dung da chon, persist qua map. Default Mouse+KB. */
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	uint8 SavedInputModeValue = 0; // 0 = MouseKeyboard, 1 = Gamepad

	/** Volume da duoc nguoi dung chon (0-1), persist qua map. */
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float SavedVolume = 1.f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentSound = nullptr;

	void PlayMusicInternal(USoundBase* NewSound);
};