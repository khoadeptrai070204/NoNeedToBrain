#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NBGameInstance.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * Game Instance giu music persist qua cac map.
 * - PlayMenuMusic / PlayMatchMusic: switch giua 2 track (cut thang, khong fade).
 * - StopMusic: tat hoan toan.
 *
 * Setup:
 *   1. Reparent BP_NBGameInstance hien tai sang NBGameInstance (Class Settings -> Parent Class).
 *      Hoac neu chua co BP, tao moi: BP class child cua NBGameInstance.
 *   2. Set MenuMusic + MatchMusic trong Class Defaults cua BP.
 *   3. Project Settings -> Maps & Modes -> Game Instance Class = BP_NBGameInstance.
 *   4. Trong WBP_MainMenu Event Construct -> Get Game Instance -> Cast -> PlayMenuMusic.
 *   5. Trong BP_NBGameMode Event BeginPlay -> Get Game Instance -> Cast -> PlayMatchMusic.
 */
UCLASS()
class NONEEDTOBRAIN_API UNBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Music phat o main menu / play menu / hero select. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TObjectPtr<USoundBase> MenuMusic = nullptr;

	/** Music phat trong tran dau. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TObjectPtr<USoundBase> MatchMusic = nullptr;

	/** Volume mac dinh cho music (0-1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MusicVolume = 0.7f;

	/** Bat dau phat menu music. Neu da phat track nay roi -> khong restart. */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void PlayMenuMusic();

	/** Bat dau phat match music. Neu da phat track nay roi -> khong restart. */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void PlayMatchMusic();

	/** Tat music hien tai. */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void StopMusic();

protected:
	/** Audio component dang phat music hien tai. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentMusicComp = nullptr;

	/** Sound dang phat (de tranh restart neu PlayXMusic goi 2 lan). */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentSound = nullptr;

	/** Helper: phat 1 sound moi (cut thang). Tu stop track cu neu co. */
	void PlayMusicInternal(USoundBase* NewSound);
};
