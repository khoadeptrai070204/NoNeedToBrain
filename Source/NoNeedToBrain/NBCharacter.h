#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "NBCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class UNBUltimateAbility;
class UNBHealthComponent;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle			UMETA(DisplayName = "Idle"),
	Attacking		UMETA(DisplayName = "Attacking"),
	Grabbing		UMETA(DisplayName = "Grabbing"),
	Throwing		UMETA(DisplayName = "Throwing"),
	UsingUltimate	UMETA(DisplayName = "UsingUltimate"),
	Stunned			UMETA(DisplayName = "Stunned"),
	Dead			UMETA(DisplayName = "Dead"),
};

UCLASS()
class NONEEDTOBRAIN_API ANBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANBCharacter();
	virtual void Tick(float DeltaSeconds) override;

	// =========================================================
	// Public state queries
	// =========================================================
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInState(ECombatState State) const { return CombatState == State; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanRotate() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanMove() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanAcceptInput() const;

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprinting() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Ultimate")
	float GetUltimateCooldownRemaining() const { return UltimateCooldownRemaining; }

	UFUNCTION(BlueprintPure, Category = "Combat|Ultimate")
	bool IsUltimateReady() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	FVector GetAimDirectionWorld() const;

	// =========================================================
	// AnimNotify hooks
	// =========================================================
	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_AttackHitWindowStart();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_AttackHitWindowEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_AttackFinished();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_ThrowRelease();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_ThrowFinished();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_UltimateFinished();

	// =========================================================
	// Grab API
	// =========================================================
	void OnGrabbedBy(ANBCharacter* InGrabber);
	void OnReleasedBy(ANBCharacter* InGrabber, bool bThrown, FVector ThrowImpulseVec);

	// =========================================================
	// Ragdoll API
	// =========================================================
	UFUNCTION(BlueprintCallable, Category = "Combat|Ragdoll")
	void EnterRagdoll(FVector InitialImpulse);

	UFUNCTION(BlueprintCallable, Category = "Combat|Ragdoll")
	void ExitRagdoll();

	UFUNCTION(BlueprintPure, Category = "Combat|Ragdoll")
	bool IsRagdolling() const { return bIsRagdoll; }

	// =========================================================
	// Public anim/state
	// =========================================================
	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	ECombatState CombatState = ECombatState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsGrabbed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsRagdoll = false;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ===== Input handlers =====
	void Input_Move(const FInputActionValue& Value);
	void Input_Aim(const FInputActionValue& Value);
	void Input_Attack(const FInputActionValue& Value);
	void Input_GrabPressed(const FInputActionValue& Value);
	void Input_GrabReleased(const FInputActionValue& Value);
	void Input_Ultimate(const FInputActionValue& Value);
	void Input_SprintPressed(const FInputActionValue& Value);
	void Input_SprintReleased(const FInputActionValue& Value);

	// ===== Rotation =====
	void UpdateAimSources(float DeltaSeconds);
	void UpdateControlRotation(float DeltaSeconds);
	bool TryGetMouseAimDirection(FVector& OutWorldDir) const;
	bool TryGetJoystickAimDirection(FVector& OutWorldDir) const;
	bool TryGetAimWorldDirection(FVector& OutWorldDir) const;

	// ===== Action requests =====
	void TryStartAttack();
	void TryStartGrab();
	void TryThrow();
	void TryUseUltimate();

	// ===== State entry/exit =====
	void StartAttack();
	void EndAttack();
	void StartGrab(ANBCharacter* Target);
	void ReleaseGrab();
	void StartThrow();
	void EndThrow();
	void StartUltimate();
	void EndUltimate();

	// ===== Helpers =====
	bool FindGrabTarget(ANBCharacter*& OutTarget) const;
	void DoAttackHitCheck();
	void TickCooldowns(float DeltaSeconds);

	/** Tính lại MaxWalkSpeed dựa trên sprint intent + grab + state. Gọi mỗi tick. */
	void UpdateMovementSpeed();

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	// ===== Anim sync =====
	void UpdateAnimSync(float DeltaSeconds);

	// =========================================================
	// Components
	// =========================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UNBHealthComponent> HealthComp = nullptr;

	// =========================================================
	// Input assets
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_Player = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Move = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Aim = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Attack = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Grab = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Ultimate = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Sprint = nullptr;

	// =========================================================
	// Rotation tuning
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	float RotationInterpSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	float AimStickDeadzone = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	bool bMouseAimPreferred = true;

	// =========================================================
	// Movement tuning
	// =========================================================
	/** Tốc độ đi bộ thường (mặc định khi không sprint). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 600.f;

	/** Tốc độ chạy khi sprint. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 950.f;

	/** Hệ số tốc độ khi đang grab (vd 0.6 = chậm 40%). Áp dụng lên WalkSpeed. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float GrabbedMoveSpeedMultiplier = 0.6f;

	// =========================================================
	// Attack tuning
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackLockSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackHitRange = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackHitRadius = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackKnockbackImpulse = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackUpwardImpulse = 200.f;

	// =========================================================
	// Grab tuning
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	float GrabRange = 110.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	float GrabRadius = 70.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	FName GrabSocketName = TEXT("GrabSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	TObjectPtr<UAnimMontage> GrabMontage = nullptr;

	// =========================================================
	// Throw tuning
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	float ThrowImpulse = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	float ThrowUpwardImpulse = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	float ThrowLockSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	TObjectPtr<UAnimMontage> ThrowMontage = nullptr;

	// =========================================================
	// Ragdoll tuning
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	float RagdollDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	FName RagdollPelvisBone = TEXT("pelvis");

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	FName RagdollCollisionProfile = TEXT("Ragdoll");

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	FName MeshDefaultCollisionProfile = TEXT("CharacterMesh");

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	TObjectPtr<UAnimMontage> GetUpMontage = nullptr;

	// =========================================================
	// Ultimate
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	TSubclassOf<UNBUltimateAbility> UltimateClass;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Ultimate")
	TObjectPtr<UNBUltimateAbility> UltimateInstance = nullptr;

private:
	// ----- Cached input -----
	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector2D LastAimStick = FVector2D::ZeroVector;
	bool bGrabHeld = false;
	bool bWantsToSprint = false;

	// ----- Aim cache -----
	bool bHasAimYaw = false;
	float DesiredAimYaw = 0.f;

	// ----- Timers -----
	FTimerHandle Timer_AttackLock;
	FTimerHandle Timer_ThrowLock;
	FTimerHandle Timer_RagdollRecover;

	// ----- Grab refs -----
	UPROPERTY()
	TWeakObjectPtr<ANBCharacter> GrabbedCharacter;

	UPROPERTY()
	TWeakObjectPtr<ANBCharacter> Grabber;

	// ----- Hit window state -----
	bool bAttackHitWindowOpen = false;
	TArray<TWeakObjectPtr<AActor>> HitThisSwing;

	// ----- Cooldowns -----
	float UltimateCooldownRemaining = 0.f;

	// ----- Cached defaults (for ragdoll restore) -----
	FVector MeshDefaultRelativeLocation = FVector::ZeroVector;
	FRotator MeshDefaultRelativeRotation = FRotator::ZeroRotator;
};