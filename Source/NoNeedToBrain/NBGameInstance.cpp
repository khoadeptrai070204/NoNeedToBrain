#include "NBGameInstance.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

void UNBGameInstance::PlayMenuMusic()
{
	if (!MenuMusic)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] MenuMusic chua set trong BP defaults"));
		return;
	}
	PlayMusicInternal(MenuMusic);
}

void UNBGameInstance::PlayMatchMusic()
{
	if (!MatchMusic)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] MatchMusic chua set trong BP defaults"));
		return;
	}
	PlayMusicInternal(MatchMusic);
}

void UNBGameInstance::StopMusic()
{
	if (CurrentMusicComp && CurrentMusicComp->IsPlaying())
	{
		CurrentMusicComp->Stop();
	}
	CurrentMusicComp = nullptr;
	CurrentSound = nullptr;
}

void UNBGameInstance::PlayMusicInternal(USoundBase* NewSound)
{
	if (!NewSound) return;

	// Neu dang phat dung track nay roi va comp van valid -> skip (tranh restart).
	if (CurrentSound == NewSound && CurrentMusicComp && CurrentMusicComp->IsPlaying())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[GameInstance] Music %s already playing, skip restart"), *NewSound->GetName());
		return;
	}

	// Stop track cu cut thang (khong fade).
	if (CurrentMusicComp && CurrentMusicComp->IsPlaying())
	{
		CurrentMusicComp->Stop();
	}

	// Spawn audio component moi.
	// SpawnSound2D vi music la non-spatial.
	// bPersistAcrossLevelTransition = true - QUAN TRONG: music khong tat khi load map.
	CurrentMusicComp = UGameplayStatics::SpawnSound2D(
		this,
		NewSound,
		MusicVolume,
		1.f,    // pitch
		0.f,    // start time
		nullptr,
		true,   // bPersistAcrossLevelTransition
		false   // bAutoDestroy - false de minh tu manage component
	);

	if (CurrentMusicComp)
	{
		CurrentSound = NewSound;
		UE_LOG(LogTemp, Log, TEXT("[GameInstance] Playing music: %s"), *NewSound->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] Failed to spawn music component!"));
	}
}
