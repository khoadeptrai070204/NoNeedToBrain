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
	GrabAttempting	UMETA(DisplayName = "GrabAttempting"),  // dang play grab anim, chua biet trung hay miss
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	// AI public API - cho AI Controller goi
	// =========================================================
	UFUNCTION(BlueprintCallable, Category = "AI")
	void PerformAIAttack();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void PerformAIGrab();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void PerformAIThrow();

	/** AI face target - snap rotation about target. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void AIFaceTarget(AActor* Target);

	/** AI bat/tat sprint. Service tu goi based on distance. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAISprinting(bool bSprint);

	/** AI jump - call Jump() built-in cua ACharacter. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void PerformAIJump();

	/** Auto-pair: class enemy character (BP_Hero_Big set la BP_Hero_Small va vice versa). */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TSubclassOf<ANBCharacter> EnemyPairClass;

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
	void Notify_KickHitWindowStart();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_KickHitWindowEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_KickFinished();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_ThrowRelease();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_ThrowFinished();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_GrabAttempt();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_GrabFinished();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies")
	void Notify_UltimateFinished();

	/** Goi khi character bi hit tu ngoai - ngat moi action dang play (attack/kick/throw). */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void InterruptCombatAction();

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
	// Public anim/state (REPLICATED for multiplayer)
	// =========================================================
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Anim")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Anim")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat")
	ECombatState CombatState = ECombatState::Idle;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat")
	bool bIsGrabbed = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat")
	bool bIsRagdoll = false;

	/** Rage gauge (0-100). Tang khi danh/bi danh. Day = duoc dung Ultimate. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Ultimate")
	float RageGauge = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	float RageMax = 100.f;

	/** Rage gain khi danh trung doi thu. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	float RageGainOnHit = 10.f;

	/** Rage gain khi bi danh trung. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	float RageGainOnTakeDamage = 15.f;

	UFUNCTION(BlueprintPure, Category = "Combat|Ultimate")
	float GetRagePercent() const { return RageGauge / FMath::Max(1.f, RageMax); }

	UFUNCTION(BlueprintPure, Category = "Combat|Ultimate")
	bool IsRageReady() const { return RageGauge >= RageMax; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	UNBHealthComponent* GetHealthComp() const { return HealthComp; }

	void AddRage(float Amount);

	/** Multicast play montage tren tat ca clients - public de Ultimate Ability goi duoc. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMontage(UAnimMontage* Montage);

	/** Multicast play montage tren tat ca clients KHONG SKIP owner (cho Ultimate va action khong client-predict). */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMontageForced(UAnimMontage* Montage);

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
	void Input_Jump(const FInputActionValue& Value);

	// ===== Server RPCs (client → server) =====
	UFUNCTION(Server, Reliable)
	void Server_RequestAttack();

	UFUNCTION(Server, Reliable)
	void Server_RequestGrab();

	UFUNCTION(Server, Reliable)
	void Server_RequestRelease();

	UFUNCTION(Server, Reliable)
	void Server_RequestUltimate();

	UFUNCTION(Server, Reliable)
	void Server_RequestJump();

	UFUNCTION(Server, Reliable)
	void Server_SetSprinting(bool bSprint);

	UFUNCTION(Server, Reliable)
	void Server_UpdateAim(float NewYaw);

	// ===== Multicast RPCs (server → all clients) for visual effects =====
	/** Multicast ragdoll state change. bEnable=true: enter ragdoll. bEnable=false: exit ragdoll. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetRagdoll(bool bEnable, FVector InitialImpulse);

	/** Multicast grabbed state - disable/enable mesh+movement on all clients. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetGrabbedState(bool bGrabbed);

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
	void StartKick();
	void EndKick();
	void BeginGrabAttempt();
	void EndGrabAttempt();
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
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Jump = nullptr;

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

	/** Van toc nhay len. Tang len 700 de co thoi gian dung kick combo. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump")
	float JumpZVelocity = 700.f;

	/** Gravity scale khi roi - giam de roi cham hon. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump")
	float JumpGravityScale = 1.8f;

	/** Air control - kha nang dieu khien khi tren khong (0=khong, 1=full). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump")
	float JumpAirControl = 0.6f;

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

	/** Thoi gian sau StartAttack -> tu trigger hit check (bypass notify AnimBP). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackHitDelay = 0.2f;

	/** Thoi gian sau hit check -> tu dong end hit window. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	float AttackHitWindowDuration = 0.15f;

	/** Montage play tren victim khi an punch. Doi voi punch, dung de tao hit feedback (thay knockback). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack")
	TObjectPtr<UAnimMontage> HitReactionMontage = nullptr;

	// =========================================================
	// Kick (jump kick - khi character tren khong + bam attack)
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	TObjectPtr<UAnimMontage> KickMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	float KickLockSeconds = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	float KickHitRange = 130.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	float KickHitRadius = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	float KickDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	float KickKnockbackImpulse = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	float KickUpwardImpulse = 400.f;

	/** Luc bay toi truoc khi kick (jump kick Mario style). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Kick")
	float KickForwardImpulse = 800.f;

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

	/** Montage loop khi da bat duoc victim - giu lien tuc den khi tha. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	TObjectPtr<UAnimMontage> GrabHoldMontage = nullptr;

	/** Montage play khi thả nạn nhân ra (animation Pull_End / GrabRelease). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	TObjectPtr<UAnimMontage> GrabReleaseMontage = nullptr;

	/** Safety timeout neu Notify_GrabFinished khong fire. Match voi grab montage length. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	float GrabAttemptTimeout = 1.0f;

	/** Thoi gian sau BeginGrabAttempt → tu trigger Notify_GrabAttempt (active frame).
	 *  Bypass notify trong AnimBP. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Grab")
	float GrabAttemptDelay = 0.3f;

	// =========================================================
	// Throw tuning
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	float ThrowImpulse = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	float ThrowUpwardImpulse = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	float ThrowLockSeconds = 0.5f;

	/** Thoi gian sau StartThrow → tu trigger ThrowRelease (detach + impulse).
	 *  Bypass notify trong AnimBP (de chac chan throw work). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	float ThrowReleaseDelay = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Throw")
	TObjectPtr<UAnimMontage> ThrowMontage = nullptr;

	// =========================================================
	// Ragdoll tuning
	// =========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	float RagdollDuration = 3.0f;

	/** Min duration ragdoll de tranh exit qua nhanh (vd victim bay len roi xuong dat ngay). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	float RagdollMinDuration = 0.8f;

	/** Velocity threshold de detect "settled" (cham dat va dung lai). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	float RagdollSettleVelocityThreshold = 50.f;

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
	FTimerHandle Timer_ThrowRelease;
	FTimerHandle Timer_GrabAttempt;
	FTimerHandle Timer_AttackHitWindow;
	FTimerHandle Timer_RagdollRecover;

	/** Time stamp ragdoll started - de tinh min duration. */
	float RagdollEnterTime = 0.f;

	// ----- Grab refs -----
	UPROPERTY()
	TWeakObjectPtr<ANBCharacter> GrabbedCharacter;

	UPROPERTY()
	TWeakObjectPtr<ANBCharacter> Grabber;

	// ----- Hit window state -----
	bool bAttackHitWindowOpen = false;
	bool bIsKicking = false;  // phan biet damage/impulse trong DoAttackHitCheck
	TArray<TWeakObjectPtr<AActor>> HitThisSwing;

	// ----- Cooldowns -----
	float UltimateCooldownRemaining = 0.f;

	// ----- Cached defaults (for ragdoll restore) -----
	FVector MeshDefaultRelativeLocation = FVector::ZeroVector;
	FRotator MeshDefaultRelativeRotation = FRotator::ZeroRotator;
};