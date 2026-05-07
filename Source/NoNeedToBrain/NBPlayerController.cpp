#include "NBPlayerController.h"
#include "NBCharacter.h"
#include "NBEndScreenWidget.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"

ANBPlayerController::ANBPlayerController()
{
	bReplicates = true;
}

void ANBPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANBPlayerController, SelectedHeroClass);
}

void ANBPlayerController::Server_SetSelectedHero_Implementation(TSubclassOf<ANBCharacter> InHeroClass)
{
	SelectedHeroClass = InHeroClass;

	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Server received hero selection: %s"),
		InHeroClass ? *InHeroClass->GetName() : TEXT("None"));
}

// =========================================================
// End Screen
// =========================================================

void ANBPlayerController::Client_ShowEndScreen_Implementation(bool bIsVictory)
{
	// RPC chay tren client cua dung PC nay -> goi ShowEndScreen local.
	ShowEndScreen(bIsVictory);
}

void ANBPlayerController::ShowEndScreen(bool bIsVictory)
{
	// Chi local controller moi can hien UI (server "host" cung la local controller cua chinh no).
	if (!IsLocalController()) return;

	// Tranh tao trung lan 2.
	if (EndScreenInstance && EndScreenInstance->IsInViewport())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] EndScreen da hien thi roi, skip."));
		return;
	}

	if (!EndScreenClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[PC] EndScreenClass chua set trong BP_NBPlayerController!"));
		return;
	}

	EndScreenInstance = CreateWidget<UNBEndScreenWidget>(this, EndScreenClass);
	if (!EndScreenInstance) return;

	EndScreenInstance->AddToViewport(100); // Z-order cao de de len HUD.
	EndScreenInstance->ShowResult(bIsVictory);

	// Chuyen sang UI mode + show cursor de bam Back duoc.
	bShowMouseCursor = true;
	FInputModeUIOnly UIMode;
	UIMode.SetWidgetToFocus(EndScreenInstance->TakeWidget());
	SetInputMode(UIMode);

	UE_LOG(LogTemp, Warning, TEXT("[PC] EndScreen shown: %s"),
		bIsVictory ? TEXT("VICTORY") : TEXT("DEFEATED"));
}