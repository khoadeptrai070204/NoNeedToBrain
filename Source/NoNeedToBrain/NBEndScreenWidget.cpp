#include "NBEndScreenWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UNBEndScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BTN_BackToMenu)
	{
		// Bind 1 lan, AddUniqueDynamic chong duplicate khi widget reconstruct.
		BTN_BackToMenu->OnClicked.AddUniqueDynamic(this, &UNBEndScreenWidget::OnBackToMenuClicked);
	}
}

void UNBEndScreenWidget::ShowResult(bool bIsVictory)
{
	if (!IMG_Result) return;

	UTexture2D* TextureToUse = bIsVictory ? T_Victory : T_Defeated;

	if (TextureToUse)
	{
		IMG_Result->SetBrushFromTexture(TextureToUse, /*bMatchSize=*/false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[NBEndScreenWidget] %s texture chua duoc set trong BP defaults!"),
			bIsVictory ? TEXT("T_Victory") : TEXT("T_Defeated"));
	}
}

void UNBEndScreenWidget::OnBackToMenuClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	// Show cursor + UI mode truoc khi load map (de menu hoat dong binh thuong).
	PC->bShowMouseCursor = true;
	FInputModeUIOnly UIMode;
	PC->SetInputMode(UIMode);

	// Open main menu map.
	UGameplayStatics::OpenLevel(this, MainMenuMapName);
}
