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

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Attacking	UMETA(DisplayName = "Attacking"),
};

UCLASS()
class NONEEDTOBRAIN_API ANBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANBCharacter();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// =========================
	// Input handlers (Enhanced Input)
	// =========================
	void Input_Move(const FInputActionValue& Value);
	void Input_Aim(const FInputActionValue& Value);      // Right stick aim vector (FVector2D)
	void Input_Attack(const FInputActionValue& Value);   // LMB / FaceButtonBottom (Started)

	// =========================
	// Rotation system (independent)
	// =========================
	void UpdateAimSources(float DeltaSeconds);
	void UpdateControlRotation(float DeltaSeconds);

	/** Tries to get a world-space aim direction from mouse cursor trace. */
	bool TryGetMouseAimDirection(FVector& OutWorldDir) const;

	/** Converts last joystick aim (FVector2D) into a world direction. */
	bool TryGetJoystickAimDirection(FVector& OutWorldDir) const;

	/** Returns true if we have a valid aim direction this frame (mouse preferred, else joystick). */
	bool TryGetAimWorldDirection(FVector& OutWorldDir) const;

	// =========================
	// Attack system (independent)
	// =========================
	void TryStartAttack();
	void StartAttack();
	void FinishAttack();

	/** Optional hook for AnimNotify to call instead of timer. Blueprint-callable for flexibility. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Notify_AttackFinished();

	bool CanAttack() const;

	// =========================
	// Animation sync (AnimBP)
	// =========================
	void UpdateAnimSync(float DeltaSeconds);

public:
	/** Current planar speed (for AnimBP). */
	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	float Speed = 0.f;

	/** Direction angle in degrees (-180..180). Works well with BlendSpace Direction. */
	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Anim")
	ECombatState CombatState = ECombatState::Idle;

protected:
	// =========================
	// Components
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera = nullptr;

	// =========================
	// Enhanced Input assets
	// =========================
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_Player = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Aim = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Attack = nullptr;

	// =========================
	// Rotation tuning
	// =========================
	/** How fast we interpolate controller yaw toward aim direction. */
	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	float RotationInterpSpeed = 12.f;

	/** Deadzone for joystick aim. */
	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	float AimStickDeadzone = 0.2f;

	/**Direction = CalculateDirection(Vel, GetActorRotation());
	 * If true: mouse aim will override stick aim whenever cursor trace hits.
	 * If false: you can change priority logic in TryGetAimWorldDirection().
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	bool bMouseAimPreferred = true;

	// =========================
	// Attack tuning
	// =========================
	/** Simple lock duration if you don't use AnimNotify. Should match montage section length. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AttackLockSeconds = 0.5f;

	/** Optional montage to play when attacking (can be null). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

private:
	// Cached input
	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector2D LastAimStick = FVector2D::ZeroVector; // stored for joystick aiming

	// Attack lock timer
	FTimerHandle Timer_AttackLock;

	// Cached desired yaw from aim
	bool bHasAimYaw = false;
	float DesiredAimYaw = 0.f;
};