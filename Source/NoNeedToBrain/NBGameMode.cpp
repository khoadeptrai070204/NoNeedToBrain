#include "NBGameMode.h"
#include "NBCharacter.h"
#include "NBHealthComponent.h"
#include "NBPlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameSession.h"

ANBGameMode::ANBGameMode()
{
	bUseSeamlessTravel = false;
}

void ANBGameMode::BeginPlay()
{
	Super::BeginPlay();
	bGameEnded = false;
	WinnerActor = nullptr;
}

FString ANBGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// Day la noi co Options chinh xac cua tung player connect.
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] InitNewPlayer - Player: %s, Options: '%s'"),
		*NewPlayerController->GetName(), *Options);

	if (ANBPlayerController* NBPC = Cast<ANBPlayerController>(NewPlayerController))
	{
		const FString HeroOption = UGameplayStatics::ParseOption(Options, TEXT("hero"));

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] InitNewPlayer - Hero option: '%s'"), *HeroOption);

		if (HeroOption.Equals(TEXT("Small"), ESearchCase::IgnoreCase) && DefaultSmallClass)
		{
			NBPC->SelectedHeroClass = DefaultSmallClass;
		}
		else if (HeroOption.Equals(TEXT("Big"), ESearchCase::IgnoreCase) && DefaultBigClass)
		{
			NBPC->SelectedHeroClass = DefaultBigClass;
		}
	}

	return Result;
}

void ANBGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] PostLogin called for %s, NetConnection: %s"),
		*NewPlayer->GetName(),
		NewPlayer->NetConnection ? TEXT("Yes (Client)") : TEXT("No (Host)"));

	// Parse URL options de tim hero choice tu client.
	if (ANBPlayerController* NBPC = Cast<ANBPlayerController>(NewPlayer))
	{
		// Host: doc URL options tu World (do Open Level luu o day).
		// Client: doc tu NetConnection URL (do Connect goi).
		FString URLString;
		if (NewPlayer->NetConnection)
		{
			// Client connect qua network - URL tu connection.
			URLString = NewPlayer->NetConnection->URL.ToString();

			// Try iterate Op array as well
			for (const FString& Opt : NewPlayer->NetConnection->URL.Op)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameMode] Client URL Op: '%s'"), *Opt);
			}
		}
		else if (UWorld* World = GetWorld())
		{
			// Host (Listen Server) - URL options tu OptionsString cua GameMode.
			URLString = OptionsString;
		}

		const FString HeroOption = UGameplayStatics::ParseOption(URLString, TEXT("hero"));

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] PostLogin - URL: '%s', Hero option: '%s'"),
			*URLString, *HeroOption);

		if (HeroOption.Equals(TEXT("Small"), ESearchCase::IgnoreCase))
		{
			NBPC->SelectedHeroClass = DefaultSmallClass;
		}
		else if (HeroOption.Equals(TEXT("Big"), ESearchCase::IgnoreCase))
		{
			NBPC->SelectedHeroClass = DefaultBigClass;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] PostLogin: Cast to NBPlayerController FAILED for %s, class=%s"),
			*NewPlayer->GetName(),
			*NewPlayer->GetClass()->GetName());
	}
}

UClass* ANBGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (ANBPlayerController* NBPC = Cast<ANBPlayerController>(InController))
	{
		// Neu da co SelectedHeroClass tu PostLogin -> dung.
		if (NBPC->SelectedHeroClass)
		{
			return NBPC->SelectedHeroClass;
		}

		// Parse URL options.
		// Voi client (NetConnection != null): dung URL.Op array de tim "hero=...".
		// Voi host (NetConnection == null): dung OptionsString cua GameMode.
		FString HeroOption;

		if (NBPC->NetConnection)
		{
			// Client: iterate Op array.
			for (const FString& Opt : NBPC->NetConnection->URL.Op)
			{
				if (Opt.StartsWith(TEXT("hero="), ESearchCase::IgnoreCase))
				{
					HeroOption = Opt.RightChop(5); // bo "hero="
					break;
				}
			}
		}
		else
		{
			// Host (Listen Server).
			HeroOption = UGameplayStatics::ParseOption(OptionsString, TEXT("hero"));
		}

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] GetDefaultPawn for %s - Hero: '%s', NetConn: %s"),
			*NBPC->GetName(), *HeroOption, NBPC->NetConnection ? TEXT("Yes") : TEXT("No"));

		if (HeroOption.Equals(TEXT("Small"), ESearchCase::IgnoreCase) && DefaultSmallClass)
		{
			NBPC->SelectedHeroClass = DefaultSmallClass;
			return DefaultSmallClass;
		}
		else if (HeroOption.Equals(TEXT("Big"), ESearchCase::IgnoreCase) && DefaultBigClass)
		{
			NBPC->SelectedHeroClass = DefaultBigClass;
			return DefaultBigClass;
		}
	}

	// Fallback default Big.
	if (DefaultBigClass)
	{
		return DefaultBigClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ANBGameMode::OnCharacterDied(ANBCharacter* DeadCharacter)
{
	if (bGameEnded) return;
	if (!DeadCharacter) return;

	CheckWinCondition();
}

void ANBGameMode::CheckWinCondition()
{
	if (bGameEnded) return;

	int32 AliveCount = 0;
	ANBCharacter* LastAlive = nullptr;

	for (TActorIterator<ANBCharacter> It(GetWorld()); It; ++It)
	{
		ANBCharacter* Char = *It;
		UNBHealthComponent* HC = Char ? Char->GetHealthComp() : nullptr;
		if (!HC) continue;

		if (HC->IsAlive())
		{
			AliveCount++;
			LastAlive = Char;
		}
	}

	if (AliveCount <= 1)
	{
		bGameEnded = true;
		WinnerActor = LastAlive;
		OnPlayerWin.Broadcast(WinnerActor);

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Game ended! Winner: %s"),
			LastAlive ? *LastAlive->GetName() : TEXT("None (Draw)"));

		// =========================================================
		// Gui Client_ShowEndScreen RPC toi tat ca PlayerControllers.
		// Server quyet dinh ai win/lose dua tren PC's Pawn == WinnerActor.
		// =========================================================
		for (FConstPlayerControllerIterator It2 = GetWorld()->GetPlayerControllerIterator(); It2; ++It2)
		{
			ANBPlayerController* NBPC = Cast<ANBPlayerController>(It2->Get());
			if (!NBPC) continue;

			// Kiem tra Pawn cua PC nay co phai winner khong.
			const bool bIsWinner = (WinnerActor != nullptr && NBPC->GetPawn() == WinnerActor);

			UE_LOG(LogTemp, Warning, TEXT("[GameMode] -> PC %s: bIsWinner=%s"),
				*NBPC->GetName(), bIsWinner ? TEXT("true") : TEXT("false"));

			NBPC->Client_ShowEndScreen(bIsWinner);
		}
	}
}