#include "NBUltimateAbility.h"
#include "NBCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWorld* UNBUltimateAbility::GetWorld() const
{
	// Required so timers / world queries work on this UObject.
	if (HasAllFlags(RF_ClassDefaultObject)) return nullptr;
	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}

bool UNBUltimateAbility::CanActivate_Implementation(ANBCharacter* Caster) const
{
	return Caster != nullptr;
}

void UNBUltimateAbility::OnActivate_Implementation(ANBCharacter* Caster)
{
	// Default behavior: play montage if any, then auto-finish via fallback timer.
	PlayMontageAndArmFallback(Caster);
}

void UNBUltimateAbility::FinishOn(ANBCharacter* Caster)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(FallbackTimerHandle);
	}
	if (Caster)
	{
		Caster->Notify_UltimateFinished();
	}
}

void UNBUltimateAbility::PlayMontageAndArmFallback(ANBCharacter* Caster)
{
	if (!Caster) return;

	if (Montage)
	{
		if (USkeletalMeshComponent* Mesh = Caster->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				Anim->Montage_Play(Montage);
			}
		}
	}

	if (FallbackDuration > 0.f)
	{
		if (UWorld* W = GetWorld())
		{
			TWeakObjectPtr<ANBCharacter> WeakCaster = Caster;
			TWeakObjectPtr<UNBUltimateAbility> WeakSelf = this;

			W->GetTimerManager().SetTimer(
				FallbackTimerHandle,
				FTimerDelegate::CreateLambda([WeakCaster, WeakSelf]()
					{
						if (WeakSelf.IsValid() && WeakCaster.IsValid())
						{
							WeakSelf->FinishOn(WeakCaster.Get());
						}
					}),
				FallbackDuration,
				false
			);
		}
	}
}