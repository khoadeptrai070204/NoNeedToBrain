#include "NBPlayerController.h"
#include "NBCharacter.h"
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
