#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NBHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, NewHealth, float, MaxHealth, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*, Killer);

/**
 * Health/HP component cho mọi thứ có thể bị đánh (player, AI, destructible…).
 * Tách thành component để dễ tái sử dụng và sạch logic khỏi character.
 *
 * MP: CurrentHealth nên Replicated, ApplyDamage nên chạy trên server.
 */
UCLASS(ClassGroup = (NB), meta = (BlueprintSpawnableComponent))
class NONEEDTOBRAIN_API UNBHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNBHealthComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	bool bIsDead = false;

	/** Số giây bất tử ngay sau khi nhận damage (chống multi-hit cùng đòn). */
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float InvulnerabilityAfterHit = 0.1f;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float Amount, AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Revive(float NewHealth);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return CurrentHealth / FMath::Max(1.f, MaxHealth); }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAlive() const { return !bIsDead; }

protected:
	virtual void BeginPlay() override;

private:
	float LastDamageTime = -1000.f;
};