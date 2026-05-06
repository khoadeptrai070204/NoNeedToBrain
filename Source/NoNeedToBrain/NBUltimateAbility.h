#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NBCharacter.h"
#include "NBUltimateAbility.generated.h"

class UAnimMontage;

/**
 * Base class for a character's ultimate ability.
 * One subclass per ultimate. Each character BP picks a class via UltimateClass.
 *
 * Override OnActivate (or OnActivate_Implementation in a BP child) to define behavior.
 * The ability is responsible for calling Caster->Notify_UltimateFinished() when it ends.
 *
 * Two ways to extend:
 *   1) C++ subclass:  override OnActivate_Implementation, optionally CanActivate_Implementation.
 *   2) Blueprint:     create BP child of UNBUltimateAbility, override OnActivate event,
 *                     call NotifyFinished on Caster from a montage AnimNotify or timer.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class NONEEDTOBRAIN_API UNBUltimateAbility : public UObject
{
	GENERATED_BODY()

public:
	/** Cooldown in seconds, started at activation time. */
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate")
	float CooldownSeconds = 30.f;

	/** Fallback duration: if the ulti doesn't call Notify_UltimateFinished within this time, the caster auto-recovers.
	 *  Subclasses can disable this by setting <= 0 and managing finish themselves. */
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate")
	float FallbackDuration = 2.5f;

	/** Optional montage played when activated. */
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ultimate")
	bool CanActivate(ANBCharacter* Caster) const;
	virtual bool CanActivate_Implementation(ANBCharacter* Caster) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ultimate")
	void OnActivate(ANBCharacter* Caster);
	virtual void OnActivate_Implementation(ANBCharacter* Caster);

	/** Helper: subclasses (or BP) call this to end the ulti. */
	UFUNCTION(BlueprintCallable, Category = "Ultimate")
	void FinishOn(ANBCharacter* Caster);

	// UObject world access
	virtual class UWorld* GetWorld() const override;

protected:
	/** Helper: play the montage on caster and arm the fallback timer. */
	void PlayMontageAndArmFallback(ANBCharacter* Caster);

	FTimerHandle FallbackTimerHandle;
};