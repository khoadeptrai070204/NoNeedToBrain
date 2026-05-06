#include "NBHealthComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UNBHealthComponent::UNBHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNBHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNBHealthComponent, CurrentHealth);
	DOREPLIFETIME(UNBHealthComponent, bIsDead);
}

void UNBHealthComponent::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, nullptr);
}

void UNBHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	bIsDead = false;
}

void UNBHealthComponent::ApplyDamage(float Amount, AActor* Instigator)
{
	// Server-authoritative: chi server moi modify CurrentHealth.
	// Client se tu sync qua OnRep_CurrentHealth.
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (bIsDead || Amount <= 0.f) return;

	// Chặn multi-hit từ cùng 1 đòn (sphere overlap có thể trùng frame).
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - LastDamageTime < InvulnerabilityAfterHit) return;
	LastDamageTime = Now;

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, Instigator);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast(Instigator);
	}
}

void UNBHealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f) return;

	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, nullptr);
}

void UNBHealthComponent::Revive(float NewHealth)
{
	bIsDead = false;
	CurrentHealth = FMath::Clamp(NewHealth, 1.f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, nullptr);
}