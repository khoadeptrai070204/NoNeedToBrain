#include "NBCharacter.h"
#include "NBUltimateAbility.h"
#include "NBHealthComponent.h"
#include "NBGameMode.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

#include "InputAction.h"
#include "InputMappingContext.h"

#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

ANBCharacter::ANBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Enable replication for multiplayer.
	bReplicates = true;
	SetReplicateMovement(true);

	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.f;
	SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	HealthComp = CreateDefaultSubobject<UNBHealthComponent>(TEXT("HealthComp"));
}

void ANBCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBCharacter, Speed);
	DOREPLIFETIME(ANBCharacter, Direction);
	DOREPLIFETIME(ANBCharacter, CombatState);
	DOREPLIFETIME(ANBCharacter, bIsGrabbed);
	DOREPLIFETIME(ANBCharacter, bIsRagdoll);
	DOREPLIFETIME(ANBCharacter, RageGauge);
}

void ANBCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Set tốc độ đi bộ ban đầu (lần đầu tiên — Tick sẽ tự cập nhật sau).
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->MaxWalkSpeed = WalkSpeed;
		Mv->JumpZVelocity = JumpZVelocity;
		Mv->GravityScale = JumpGravityScale;
		Mv->AirControl = JumpAirControl;
	}
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		MeshDefaultRelativeLocation = SkelMesh->GetRelativeLocation();
		MeshDefaultRelativeRotation = SkelMesh->GetRelativeRotation();
	}

	if (HealthComp)
	{
		HealthComp->OnDeath.AddDynamic(this, &ANBCharacter::HandleDeath);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (IMC_Player) Sub->AddMappingContext(IMC_Player, 0);
			}
		}
	}

	if (UltimateClass)
	{
		UltimateInstance = NewObject<UNBUltimateAbility>(this, UltimateClass);
	}
}

void ANBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAimSources(DeltaSeconds);
	UpdateControlRotation(DeltaSeconds);
	UpdateMovementSpeed();
	UpdateAnimSync(DeltaSeconds);
	TickCooldowns(DeltaSeconds);

	if (bAttackHitWindowOpen)
	{
		DoAttackHitCheck();
	}

	// Server-only: trong khi ragdoll, follow actor theo pelvis bone.
	// Khi exit ragdoll, actor da o dung vi tri mesh, khong can teleport → smooth.
	if (HasAuthority() && bIsRagdoll)
	{
		USkeletalMeshComponent* SkelMesh = GetMesh();
		UCapsuleComponent* Cap = GetCapsuleComponent();
		if (SkelMesh && Cap)
		{
			const FVector PelvisLoc = SkelMesh->GetSocketLocation(RagdollPelvisBone);
			const FVector ActorLoc = GetActorLocation();

			// Safety: chi update actor neu pelvis chua qua thap (tranh xuyen san).
			if (PelvisLoc.Z > ActorLoc.Z - 200.f)
			{
				const float CapHalf = Cap->GetScaledCapsuleHalfHeight();
				const FVector NewLoc = PelvisLoc + FVector(0.f, 0.f, CapHalf);
				SetActorLocation(NewLoc, false, nullptr, ETeleportType::TeleportPhysics);
			}

			// Early exit ragdoll khi settle (cham dat + dung lai) + da qua min duration.
			const float TimeSinceEnter = GetWorld()->GetTimeSeconds() - RagdollEnterTime;
			if (TimeSinceEnter >= RagdollMinDuration)
			{
				const FVector PelvisVel = SkelMesh->GetPhysicsLinearVelocity(RagdollPelvisBone);
				if (PelvisVel.Size() < RagdollSettleVelocityThreshold)
				{
					// Da dung lai - exit ragdoll som.
					ExitRagdoll();
				}
			}
		}
	}
}

void ANBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	check(EIC);

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ANBCharacter::Input_Move);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &ANBCharacter::Input_Move);
	}
	if (IA_Aim)
	{
		EIC->BindAction(IA_Aim, ETriggerEvent::Triggered, this, &ANBCharacter::Input_Aim);
		EIC->BindAction(IA_Aim, ETriggerEvent::Completed, this, &ANBCharacter::Input_Aim);
	}
	if (IA_Attack)
	{
		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ANBCharacter::Input_Attack);
	}
	if (IA_Grab)
	{
		EIC->BindAction(IA_Grab, ETriggerEvent::Started, this, &ANBCharacter::Input_GrabPressed);
		EIC->BindAction(IA_Grab, ETriggerEvent::Completed, this, &ANBCharacter::Input_GrabReleased);
	}
	if (IA_Ultimate)
	{
		EIC->BindAction(IA_Ultimate, ETriggerEvent::Started, this, &ANBCharacter::Input_Ultimate);
	}
	if (IA_Sprint)
	{
		// Hold-to-sprint: Started khi bấm xuống, Completed khi thả.
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ANBCharacter::Input_SprintPressed);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ANBCharacter::Input_SprintReleased);
	}
	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ANBCharacter::Input_Jump);
	}
}

// =========================================================
// State queries
// =========================================================

bool ANBCharacter::CanAcceptInput() const
{
	if (bIsGrabbed || bIsRagdoll) return false;
	if (CombatState == ECombatState::Dead) return false;
	if (CombatState == ECombatState::Stunned) return false;
	return true;
}

bool ANBCharacter::CanRotate() const
{
	if (!CanAcceptInput()) return false;
	switch (CombatState)
	{
	case ECombatState::Idle:          return true;
	case ECombatState::Grabbing:      return true;
	case ECombatState::Attacking:     return false;
	case ECombatState::Throwing:      return false;
	case ECombatState::UsingUltimate: return false;
	default:                          return false;
	}
}

bool ANBCharacter::CanMove() const
{
	if (!CanAcceptInput()) return false;
	switch (CombatState)
	{
	case ECombatState::Idle:          return true;
	case ECombatState::Grabbing:      return true;
	case ECombatState::Attacking:     return false;
	case ECombatState::Throwing:      return false;
	case ECombatState::UsingUltimate: return false;
	default:                          return false;
	}
}

bool ANBCharacter::IsSprinting() const
{
	// Sprint thực sự khi: muốn sprint + đang đi (có velocity) + state cho phép + KHÔNG đang grab.
	if (!bWantsToSprint) return false;
	if (!CanMove()) return false;
	if (CombatState == ECombatState::Grabbing) return false;
	return GetVelocity().SizeSquared2D() > 100.f; // có thực sự di chuyển không
}

bool ANBCharacter::IsUltimateReady() const
{
	// Ultimate ready = co UltimateInstance + Rage day (thay vi cooldown timer).
	return UltimateInstance && IsRageReady();
}

void ANBCharacter::AddRage(float Amount)
{
	if (!HasAuthority()) return;
	RageGauge = FMath::Clamp(RageGauge + Amount, 0.f, RageMax);
}

FVector ANBCharacter::GetAimDirectionWorld() const
{
	if (bHasAimYaw) return FRotator(0.f, DesiredAimYaw, 0.f).Vector();
	return GetActorForwardVector();
}

// =========================================================
// AI public API
// =========================================================

void ANBCharacter::PerformAIAttack()
{
	if (!CanAcceptInput()) return;
	if (CombatState != ECombatState::Idle) return;

	// AI dung punch (mat dat) - khong dung kick (jump kick can airborne).
	StartAttack();
}

void ANBCharacter::PerformAIGrab()
{
	if (!CanAcceptInput()) return;
	if (CombatState != ECombatState::Idle) return;
	BeginGrabAttempt();
}

void ANBCharacter::PerformAIThrow()
{
	if (CombatState != ECombatState::Grabbing) return;
	if (!GrabbedCharacter.IsValid()) return;
	StartThrow();
}

void ANBCharacter::AIFaceTarget(AActor* Target)
{
	if (!Target || !Controller) return;

	// Tinh huong tu AI toi target.
	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero()) return;

	const float Yaw = ToTarget.Rotation().Yaw;
	DesiredAimYaw = Yaw;
	bHasAimYaw = true;

	// Snap rotation ngay (khong interp) - AI quay nhanh hon player.
	Controller->SetControlRotation(FRotator(0.f, Yaw, 0.f));
}

void ANBCharacter::SetAISprinting(bool bSprint)
{
	bWantsToSprint = bSprint;
}

void ANBCharacter::PerformAIJump()
{
	if (!CanAcceptInput()) return;
	if (CombatState != ECombatState::Idle) return;

	UCharacterMovementComponent* Mv = GetCharacterMovement();
	if (Mv && !Mv->IsFalling())
	{
		Jump();  // ACharacter built-in
	}
}

// =========================================================
// Input
// =========================================================

void ANBCharacter::Input_Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (!Controller || !CanMove()) return;

	// World-space input cho top-down game:
	// W luôn đi về phía Y+ (forward world), không phụ thuộc hướng character đang nhìn.
	// Điều này tránh glitch "đi theo cung tròn" khi character đang xoay theo aim.
	const FVector Forward = FVector::ForwardVector;  // (1, 0, 0) world
	const FVector Right = FVector::RightVector;    // (0, 1, 0) world

	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right, MoveInput.X);
}

void ANBCharacter::Input_Aim(const FInputActionValue& Value)
{
	if (!CanAcceptInput()) return;
	const FVector2D Aim = Value.Get<FVector2D>();
	if (Aim.SizeSquared() >= FMath::Square(AimStickDeadzone))
	{
		LastAimStick = Aim;
	}
}

void ANBCharacter::Input_Attack(const FInputActionValue&)
{
	// CLIENT-SIDE PREDICTION: owner client play montage NGAY (smooth, no jitter).
	// Server xu ly logic + damage authoritative.
	if (IsLocallyControlled() && !HasAuthority())
	{
		if (CanAcceptInput())
		{
			UAnimMontage* PredictedMontage = nullptr;

			// Grabbing → click attack = throw.
			if (CombatState == ECombatState::Grabbing && ThrowMontage)
			{
				PredictedMontage = ThrowMontage;
			}
			else if (CombatState == ECombatState::Idle)
			{
				UCharacterMovementComponent* Mv = GetCharacterMovement();
				if (Mv && Mv->IsFalling() && KickMontage)
				{
					PredictedMontage = KickMontage;
				}
				else if (AttackMontage)
				{
					PredictedMontage = AttackMontage;
				}
			}

			if (PredictedMontage)
			{
				if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					Anim->Montage_Play(PredictedMontage);
				}
			}
		}
	}

	// Gui request len server (server xu ly state + damage + impulse).
	Server_RequestAttack();
}

void ANBCharacter::Input_Jump(const FInputActionValue&)
{
	Server_RequestJump();
}

void ANBCharacter::Input_GrabPressed(const FInputActionValue&)
{
	// Client predict: play grab montage local.
	if (IsLocallyControlled() && !HasAuthority())
	{
		if (CanAcceptInput() && CombatState == ECombatState::Idle && GrabMontage)
		{
			if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
			{
				Anim->Montage_Play(GrabMontage);
			}
		}
	}
	Server_RequestGrab();
}

void ANBCharacter::Input_GrabReleased(const FInputActionValue&)
{
	Server_RequestRelease();
}

void ANBCharacter::Input_Ultimate(const FInputActionValue&)
{
	Server_RequestUltimate();
}

void ANBCharacter::Input_SprintPressed(const FInputActionValue&)
{
	Server_SetSprinting(true);
}

void ANBCharacter::Input_SprintReleased(const FInputActionValue&)
{
	Server_SetSprinting(false);
}

// =========================================================
// Movement speed — single source of truth
// =========================================================

void ANBCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* Mv = GetCharacterMovement();
	if (!Mv) return;

	float TargetSpeed = WalkSpeed;

	// Ưu tiên 1: đang grab → áp slow lên WalkSpeed (sprint không có tác dụng).
	if (CombatState == ECombatState::Grabbing)
	{
		TargetSpeed = WalkSpeed * GrabbedMoveSpeedMultiplier;
	}
	// Ưu tiên 2: muốn sprint + state cho phép.
	else if (bWantsToSprint && CanMove())
	{
		TargetSpeed = SprintSpeed;
	}

	Mv->MaxWalkSpeed = TargetSpeed;
}

// =========================================================
// Rotation
// =========================================================

void ANBCharacter::UpdateAimSources(float)
{
	FVector AimDirWorld;
	if (TryGetAimWorldDirection(AimDirWorld))
	{
		DesiredAimYaw = AimDirWorld.Rotation().Yaw;
		bHasAimYaw = true;
	}
	else
	{
		bHasAimYaw = false;
	}
}

void ANBCharacter::UpdateControlRotation(float DeltaSeconds)
{
	if (!bHasAimYaw || !CanRotate()) return;

	AController* C = GetController();
	if (!C) return;

	const FRotator Current = C->GetControlRotation();
	const FRotator Target(0.f, DesiredAimYaw, 0.f);
	const FRotator NewRot = FMath::RInterpTo(Current, Target, DeltaSeconds, RotationInterpSpeed);
	C->SetControlRotation(NewRot);
}

bool ANBCharacter::TryGetMouseAimDirection(FVector& OutWorldDir) const
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return false;

	FHitResult Hit;
	if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit)) return false;

	FVector Dir = Hit.ImpactPoint - GetActorLocation();
	Dir.Z = 0.f;
	if (Dir.IsNearlyZero()) return false;

	OutWorldDir = Dir.GetSafeNormal();
	return true;
}

bool ANBCharacter::TryGetJoystickAimDirection(FVector& OutWorldDir) const
{
	if (LastAimStick.SizeSquared() < FMath::Square(AimStickDeadzone)) return false;
	const FVector Dir(LastAimStick.Y, -LastAimStick.X, 0.f);
	if (Dir.IsNearlyZero()) return false;
	OutWorldDir = Dir.GetSafeNormal();
	return true;
}

bool ANBCharacter::TryGetAimWorldDirection(FVector& OutWorldDir) const
{
	if (bMouseAimPreferred)
	{
		if (TryGetMouseAimDirection(OutWorldDir)) return true;
		return TryGetJoystickAimDirection(OutWorldDir);
	}
	if (TryGetJoystickAimDirection(OutWorldDir)) return true;
	return TryGetMouseAimDirection(OutWorldDir);
}

// =========================================================
// Attack
// =========================================================

void ANBCharacter::TryStartAttack()
{
	if (CombatState != ECombatState::Idle) return;
	StartAttack();
}

void ANBCharacter::StartAttack()
{
	CombatState = ECombatState::Attacking;
	HitThisSwing.Reset();
	bAttackHitWindowOpen = false;
	bIsKicking = false;

	// Stop velocity ngay lap tuc - dam bao character khong tiep tuc truot khi attack.
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->StopMovementImmediately();
	}

	if (bHasAimYaw)
	{
		if (AController* C = GetController())
		{
			C->SetControlRotation(FRotator(0.f, DesiredAimYaw, 0.f));
		}
	}

	if (AttackMontage)
	{
		Multicast_PlayMontage(AttackMontage);
	}

	// Timer workaround: tu mo hit window sau AttackHitDelay (bypass notify AnimBP).
	GetWorldTimerManager().ClearTimer(Timer_AttackHitWindow);
	FTimerDelegate StartDel;
	StartDel.BindUFunction(this, FName("Notify_AttackHitWindowStart"));
	GetWorldTimerManager().SetTimer(Timer_AttackHitWindow, StartDel, AttackHitDelay, false);

	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	GetWorldTimerManager().SetTimer(
		Timer_AttackLock, this, &ANBCharacter::EndAttack, AttackLockSeconds, false);
}

void ANBCharacter::EndAttack()
{
	bAttackHitWindowOpen = false;
	bIsKicking = false;
	HitThisSwing.Reset();
	if (CombatState == ECombatState::Attacking)
	{
		CombatState = ECombatState::Idle;
	}
}

// =========================================================
// Kick (jump kick)
// =========================================================

void ANBCharacter::StartKick()
{
	CombatState = ECombatState::Attacking;  // dung chung state Attacking de lock movement
	HitThisSwing.Reset();
	bAttackHitWindowOpen = false;
	bIsKicking = true;

	// Snap rotation theo aim.
	if (bHasAimYaw)
	{
		if (AController* C = GetController())
		{
			C->SetControlRotation(FRotator(0.f, DesiredAimYaw, 0.f));
		}
	}

	// Apply forward impulse + freeze in air neu dang tren khong (jump kick Mario style).
	UCharacterMovementComponent* Mv = GetCharacterMovement();
	if (Mv)
	{
		const bool bWasFalling = Mv->IsFalling();

		// Forward impulse - bay toi truoc (chi neu KickForwardImpulse > 0).
		if (KickForwardImpulse > 0.f)
		{
			const FVector ForwardImpulse = GetActorForwardVector() * KickForwardImpulse;
			Mv->AddImpulse(ForwardImpulse, true);
		}

		// Neu dang tren khong, freeze gravity + cat het momentum de character dung yen khi kick.
		if (bWasFalling)
		{
			Mv->GravityScale = 0.f;
			// Cat het velocity (X, Y, Z) - khong giu momentum khi kick.
			Mv->Velocity = FVector::ZeroVector;
		}
	}

	if (KickMontage)
	{
		Multicast_PlayMontage(KickMontage);
	}

	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	GetWorldTimerManager().SetTimer(
		Timer_AttackLock, this, &ANBCharacter::EndKick, KickLockSeconds, false);
}

void ANBCharacter::EndKick()
{
	bAttackHitWindowOpen = false;
	bIsKicking = false;
	HitThisSwing.Reset();

	// Restore gravity - character bat dau roi xuong tu vi tri kick.
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->GravityScale = JumpGravityScale;
	}

	if (CombatState == ECombatState::Attacking)
	{
		CombatState = ECombatState::Idle;
	}
}

void ANBCharacter::Notify_KickHitWindowStart()
{
	if (CombatState != ECombatState::Attacking || !bIsKicking) return;
	bAttackHitWindowOpen = true;
	HitThisSwing.Reset();
}

void ANBCharacter::Notify_KickHitWindowEnd()
{
	bAttackHitWindowOpen = false;
}

void ANBCharacter::Notify_KickFinished()
{
	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	EndKick();
}

// =========================================================
// Interrupt - khi bi hit luc dang attack/kick/throw thi cancel
// =========================================================

void ANBCharacter::InterruptCombatAction()
{
	// Khong interrupt neu da Stunned/Dead/Ragdoll - de logic khac handle.
	if (CombatState == ECombatState::Stunned ||
		CombatState == ECombatState::Dead ||
		bIsRagdoll)
	{
		return;
	}

	// Stop tat ca montage dang play.
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		Anim->StopAllMontages(0.15f);  // blend out 0.15s cho do giat
	}

	// Clear timers.
	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	GetWorldTimerManager().ClearTimer(Timer_ThrowLock);

	// Neu dang grab ai do thi tha ra.
	if (GrabbedCharacter.IsValid())
	{
		ReleaseGrab();
	}

	// Neu dang kick (gravity = 0) thi restore gravity de roi xuong.
	if (bIsKicking)
	{
		if (UCharacterMovementComponent* Mv = GetCharacterMovement())
		{
			Mv->GravityScale = JumpGravityScale;
		}
	}

	// Reset hit window state.
	bAttackHitWindowOpen = false;
	bIsKicking = false;
	HitThisSwing.Reset();

	// Reset state ve Idle (chi khi dang trong combat action).
	switch (CombatState)
	{
	case ECombatState::Attacking:
	case ECombatState::Throwing:
	case ECombatState::UsingUltimate:
	case ECombatState::GrabAttempting:
		CombatState = ECombatState::Idle;
		break;
	default:
		break;
	}
}

void ANBCharacter::Notify_AttackHitWindowStart()
{
	if (CombatState != ECombatState::Attacking) return;
	bAttackHitWindowOpen = true;
	HitThisSwing.Reset();

	// Tu schedule end window sau AttackHitWindowDuration.
	FTimerDelegate EndDel;
	EndDel.BindUFunction(this, FName("Notify_AttackHitWindowEnd"));
	GetWorldTimerManager().SetTimer(Timer_AttackHitWindow, EndDel, AttackHitWindowDuration, false);
}

void ANBCharacter::Notify_AttackHitWindowEnd()
{
	bAttackHitWindowOpen = false;
}

void ANBCharacter::Notify_AttackFinished()
{
	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	EndAttack();
}

void ANBCharacter::DoAttackHitCheck()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Range/radius khac nhau cho kick va punch.
	const float HitRange = bIsKicking ? KickHitRange : AttackHitRange;
	const float HitRadius = bIsKicking ? KickHitRadius : AttackHitRadius;
	const float Damage = bIsKicking ? KickDamage : AttackDamage;
	const float Knockback = bIsKicking ? KickKnockbackImpulse : AttackKnockbackImpulse;
	const float Upward = bIsKicking ? KickUpwardImpulse : AttackUpwardImpulse;

	const FVector Start = GetActorLocation() + GetActorForwardVector() * HitRange * 0.5f;
	FCollisionShape Shape = FCollisionShape::MakeSphere(HitRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NB_AttackHit), false, this);
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, Start, FQuat::Identity, ECC_Pawn, Shape, Params);

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* A = O.GetActor();
		if (!A || A == this) continue;

		bool bAlready = false;
		for (const TWeakObjectPtr<AActor>& W : HitThisSwing)
		{
			if (W.Get() == A) { bAlready = true; break; }
		}
		if (bAlready) continue;
		HitThisSwing.Add(A);

		if (ANBCharacter* Other = Cast<ANBCharacter>(A))
		{
			// Neu victim dang trong combat action (Attack/Kick/Throw/Ulti/GrabAttempt) - ngat hanh dong.
			if (Other->CombatState == ECombatState::Attacking ||
				Other->CombatState == ECombatState::Throwing ||
				Other->CombatState == ECombatState::UsingUltimate ||
				Other->CombatState == ECombatState::GrabAttempting)
			{
				Other->InterruptCombatAction();
			}

			if (Other->HealthComp)
			{
				Other->HealthComp->ApplyDamage(Damage, this);
			}

			// Rage system: attacker gain rage tren hit, victim gain rage khi bi hit.
			AddRage(RageGainOnHit);
			Other->AddRage(Other->RageGainOnTakeDamage);

			// Punch: hit reaction.
			// Kick: knockback (day bay) - khong ragdoll.
			if (bIsKicking)
			{
				FVector Push = Other->GetActorLocation() - GetActorLocation();
				Push.Z = 0.f;
				Push = Push.GetSafeNormal();
				const FVector Impulse = Push * Knockback + FVector::UpVector * Upward;

				if (UCharacterMovementComponent* VictimMv = Other->GetCharacterMovement())
				{
					VictimMv->AddImpulse(Impulse, true);
				}

				// Optional: play hit reaction de victim biet bi danh (KHONG ragdoll).
				if (Other->HitReactionMontage)
				{
					Other->Multicast_PlayMontage(Other->HitReactionMontage);
				}
			}
			else
			{
				// Punch: stop victim velocity + light push + play hit reaction.
				if (UCharacterMovementComponent* VictimMv = Other->GetCharacterMovement())
				{
					VictimMv->StopMovementImmediately();

					FVector PushDir = Other->GetActorLocation() - GetActorLocation();
					PushDir.Z = 0.f;
					PushDir = PushDir.GetSafeNormal();
					const FVector LightImpulse = PushDir * (Knockback * 0.3f);
					VictimMv->AddImpulse(LightImpulse, true);
				}

				if (Other->HitReactionMontage)
				{
					Other->Multicast_PlayMontage(Other->HitReactionMontage);
				}
			}

			if (Other->CombatState == ECombatState::Grabbing)
			{
				Other->ReleaseGrab();
			}
		}
	}
}

// =========================================================
// Grab / Throw
// =========================================================

void ANBCharacter::TryStartGrab()
{
	// Cho phep grab khi Idle. Animation luon play, target check sau o notify GrabAttempt.
	if (CombatState != ECombatState::Idle) return;

	BeginGrabAttempt();
}

void ANBCharacter::BeginGrabAttempt()
{
	CombatState = ECombatState::GrabAttempting;

	// Stop velocity ngay - khoa movement trong khi grab anim play.
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->StopMovementImmediately();
	}

	// Snap rotation theo aim de grab dung huong.
	if (bHasAimYaw)
	{
		if (AController* C = GetController())
		{
			C->SetControlRotation(FRotator(0.f, DesiredAimYaw, 0.f));
		}
	}

	// Play grab montage NGAY - du chua biet trung hay miss.
	if (GrabMontage)
	{
		Multicast_PlayMontage(GrabMontage);
	}

	// Timer GrabAttempt: bypass notify trong AnimBP - tu trigger active frame.
	GetWorldTimerManager().ClearTimer(Timer_GrabAttempt);
	GetWorldTimerManager().SetTimer(
		Timer_GrabAttempt, this, &ANBCharacter::Notify_GrabAttempt, GrabAttemptDelay, false);

	// Safety timer - neu Notify_GrabFinished khong fire thi tu reset Idle.
	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	GetWorldTimerManager().SetTimer(
		Timer_AttackLock, this, &ANBCharacter::EndGrabAttempt, GrabAttemptTimeout, false);
}

void ANBCharacter::EndGrabAttempt()
{
	// Clear timer grab attempt.
	GetWorldTimerManager().ClearTimer(Timer_GrabAttempt);

	// Chi reset neu van dang GrabAttempting (chua trung target).
	if (CombatState == ECombatState::GrabAttempting)
	{
		CombatState = ECombatState::Idle;
	}
}

void ANBCharacter::Notify_GrabAttempt()
{
	// Active frame - bay gio moi check target.
	if (CombatState != ECombatState::GrabAttempting) return;

	ANBCharacter* Target = nullptr;
	if (FindGrabTarget(Target) && Target)
	{
		// Trung target - chuyen sang Grabbing state, attach victim.
		StartGrab(Target);
	}
	// Khong trung - khong lam gi, animation tiep tuc play den het.
}

void ANBCharacter::Notify_GrabFinished()
{
	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	EndGrabAttempt();
}

bool ANBCharacter::FindGrabTarget(ANBCharacter*& OutTarget) const
{
	OutTarget = nullptr;
	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector Origin = GetActorLocation() + GetActorForwardVector() * GrabRange * 0.5f;
	FCollisionShape Shape = FCollisionShape::MakeSphere(GrabRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NB_GrabFind), false, this);
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Shape, Params);

	float BestDist = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		ANBCharacter* C = Cast<ANBCharacter>(O.GetActor());
		if (!C || C == this) continue;
		if (C->bIsGrabbed || C->bIsRagdoll || C->CombatState == ECombatState::Dead) continue;

		const float D = FVector::DistSquared(GetActorLocation(), C->GetActorLocation());
		if (D < BestDist)
		{
			BestDist = D;
			OutTarget = C;
		}
	}
	return OutTarget != nullptr;
}

void ANBCharacter::StartGrab(ANBCharacter* Target)
{
	if (!Target) return;

	// Stop safety timer cua grab attempt - victim da bat duoc.
	GetWorldTimerManager().ClearTimer(Timer_AttackLock);

	CombatState = ECombatState::Grabbing;
	GrabbedCharacter = Target;
	Target->OnGrabbedBy(this);

	// Attach: snap location vao socket, NHUNG keep world rotation cua victim
	// de tranh victim bi nghieng theo grabber (vd grabber nhin xuong cursor → victim cung nghieng).
	// Sau detach, victim van giu rotation upright thay vi nam ngang.
	const FAttachmentTransformRules Rules(
		EAttachmentRule::SnapToTarget,    // Location: snap vao socket
		EAttachmentRule::KeepWorld,        // Rotation: GIU NGUYEN (khong copy tu grabber)
		EAttachmentRule::KeepWorld,        // Scale: giu nguyen
		true);
	Target->AttachToComponent(GetMesh(), Rules, GrabSocketName);

	// Speed se tu update trong Tick → UpdateMovementSpeed.

	// Stop grab attempt montage local (montage da multicast nen client tu stop khi animation het).
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (GrabMontage && Anim->Montage_IsPlaying(GrabMontage))
		{
			Anim->Montage_Stop(0.1f, GrabMontage);
		}
	}
	// Multicast hold montage to all clients.
	if (GrabHoldMontage)
	{
		Multicast_PlayMontage(GrabHoldMontage);
	}
}

void ANBCharacter::ReleaseGrab()
{
	if (ANBCharacter* V = GrabbedCharacter.Get())
	{
		V->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
		V->OnReleasedBy(this, false, FVector::ZeroVector);
	}
	GrabbedCharacter = nullptr;

	// Stop GrabHoldMontage local + multicast release montage.
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (GrabHoldMontage && Anim->Montage_IsPlaying(GrabHoldMontage))
		{
			Anim->Montage_Stop(0.15f, GrabHoldMontage);
		}
	}
	if (GrabReleaseMontage)
	{
		Multicast_PlayMontage(GrabReleaseMontage);
	}

	if (CombatState == ECombatState::Grabbing)
	{
		CombatState = ECombatState::Idle;
	}
	// Speed restore tự động trong Tick.
}

void ANBCharacter::TryThrow()
{
	if (CombatState != ECombatState::Grabbing) return;
	if (!GrabbedCharacter.IsValid()) { ReleaseGrab(); return; }
	StartThrow();
}

void ANBCharacter::StartThrow()
{
	CombatState = ECombatState::Throwing;

	// Lock movement + rotation: stop velocity ngay, snap rotation theo aim hien tai.
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->StopMovementImmediately();
	}
	if (bHasAimYaw)
	{
		if (AController* C = GetController())
		{
			C->SetControlRotation(FRotator(0.f, DesiredAimYaw, 0.f));
		}
	}

	if (ThrowMontage)
	{
		Multicast_PlayMontage(ThrowMontage);
	}

	// Timer ThrowRelease: bypass notify trong AnimBP - tu trigger sau ThrowReleaseDelay.
	GetWorldTimerManager().ClearTimer(Timer_ThrowRelease);
	GetWorldTimerManager().SetTimer(
		Timer_ThrowRelease, this, &ANBCharacter::Notify_ThrowRelease, ThrowReleaseDelay, false);

	// Timer ThrowLock: safety - sau ThrowLockSeconds tu reset state.
	GetWorldTimerManager().ClearTimer(Timer_ThrowLock);
	GetWorldTimerManager().SetTimer(
		Timer_ThrowLock, this, &ANBCharacter::EndThrow, ThrowLockSeconds, false);
}

void ANBCharacter::Notify_ThrowRelease()
{
	if (CombatState != ECombatState::Throwing) return;
	if (!GrabbedCharacter.IsValid()) return;

	ANBCharacter* V = GrabbedCharacter.Get();

	// Detach victim. KeepWorld giu vi tri hien tai.
	V->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));

	V->bIsGrabbed = false;
	V->SetReplicateMovement(true);
	V->bUseControllerRotationYaw = true;

	// Multicast re-enable physics + movement tren TAT CA clients.
	V->Multicast_SetGrabbedState(false);

	// Reset rotation upright.
	{
		FRotator UprightRot(0.f, V->GetActorRotation().Yaw, 0.f);
		V->SetActorRotation(UprightRot, ETeleportType::TeleportPhysics);
	}

	// Apply impulse VAO CHARACTER MOVEMENT (khong ragdoll mesh).
	// Bay ngang theo aim direction, gravity tu xu ly roi xuong dat.
	if (UCharacterMovementComponent* VMv = V->GetCharacterMovement())
	{
		VMv->SetMovementMode(MOVE_Falling);

		const FVector Dir = GetAimDirectionWorld();
		const FVector Impulse = Dir * ThrowImpulse + FVector::UpVector * ThrowUpwardImpulse;
		VMv->AddImpulse(Impulse, true);
	}

	V->CombatState = ECombatState::Idle;
	GrabbedCharacter = nullptr;
}

void ANBCharacter::Notify_ThrowFinished()
{
	GetWorldTimerManager().ClearTimer(Timer_ThrowLock);
	EndThrow();
}

void ANBCharacter::EndThrow()
{
	// Clear ca 2 timers
	GetWorldTimerManager().ClearTimer(Timer_ThrowLock);
	GetWorldTimerManager().ClearTimer(Timer_ThrowRelease);

	if (GrabbedCharacter.IsValid())
	{
		ReleaseGrab();
		return;
	}
	if (CombatState == ECombatState::Throwing)
	{
		CombatState = ECombatState::Idle;
	}
}

// =========================================================
// Victim-side
// =========================================================

void ANBCharacter::OnGrabbedBy(ANBCharacter* InGrabber)
{
	bIsGrabbed = true;
	Grabber = InGrabber;
	CombatState = ECombatState::Stunned;

	// Stop replicate movement va rotation tu server.
	SetReplicateMovement(false);
	bUseControllerRotationYaw = false;

	// Multicast disable physics + movement tren TAT CA clients (kha quan trong - tranh fight position).
	Multicast_SetGrabbedState(true);
}

void ANBCharacter::Multicast_SetGrabbedState_Implementation(bool bGrabbed)
{
	if (bGrabbed)
	{
		// Disable movement component (khong tinh toan velocity, gravity, etc.).
		if (UCharacterMovementComponent* Mv = GetCharacterMovement())
		{
			Mv->StopMovementImmediately();
			Mv->DisableMovement();
			Mv->SetComponentTickEnabled(false);
		}
		// Disable capsule collision (no physics interaction).
		if (UCapsuleComponent* Cap = GetCapsuleComponent())
		{
			Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		// Disable mesh collision.
		if (USkeletalMeshComponent* SkelMesh = GetMesh())
		{
			SkelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		// Re-enable everything.
		if (UCharacterMovementComponent* Mv = GetCharacterMovement())
		{
			Mv->SetComponentTickEnabled(true);
			Mv->SetMovementMode(MOVE_Walking);
		}
		if (UCapsuleComponent* Cap = GetCapsuleComponent())
		{
			Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		if (USkeletalMeshComponent* SkelMesh = GetMesh())
		{
			SkelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}

void ANBCharacter::OnReleasedBy(ANBCharacter*, bool bThrown, FVector ThrowImpulseVec)
{
	bIsGrabbed = false;
	Grabber = nullptr;

	// Re-enable replicate movement (da disable khi grab).
	SetReplicateMovement(true);
	bUseControllerRotationYaw = true;

	// Multicast re-enable physics + movement tren TAT CA clients.
	Multicast_SetGrabbedState(false);

	// CRITICAL: Reset Actor rotation ve upright (Roll=0, Pitch=0).
	// Tranh case sau nhieu lan grab/release, rotation bi drift tich tu.
	{
		FRotator CurrentRot = GetActorRotation();
		FRotator UprightRot(0.f, CurrentRot.Yaw, 0.f);
		SetActorRotation(UprightRot, ETeleportType::TeleportPhysics);
	}

	// Reset Mesh relative transform ve default - tranh mesh bi nghieng.
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetRelativeLocation(MeshDefaultRelativeLocation);
		SkelMesh->SetRelativeRotation(MeshDefaultRelativeRotation);
	}

	// Throw: ragdoll bay NGAY (khong qua Falling state - tranh roi 1 frame).
	// Release thuong: dung ngay, khong ragdoll.
	if (bThrown)
	{
		EnterRagdoll(ThrowImpulseVec);
	}
	else
	{
		// Release thuong: re-enable movement, character van dung yen.
		if (UCharacterMovementComponent* Mv = GetCharacterMovement())
		{
			Mv->SetMovementMode(MOVE_Walking);
		}
		CombatState = ECombatState::Idle;
	}
}

// =========================================================
// Ragdoll
// =========================================================

void ANBCharacter::EnterRagdoll(FVector InitialImpulse)
{
	if (bIsRagdoll) return;
	bIsRagdoll = true;
	CombatState = ECombatState::Stunned;
	RagdollEnterTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (GrabbedCharacter.IsValid())
	{
		ReleaseGrab();
	}

	// Multicast den all clients de set physics + impulse.
	Multicast_SetRagdoll(true, InitialImpulse);

	// Server-only: timer recover (max duration safety).
	GetWorldTimerManager().ClearTimer(Timer_RagdollRecover);
	GetWorldTimerManager().SetTimer(
		Timer_RagdollRecover, this, &ANBCharacter::ExitRagdoll, RagdollDuration, false);
}

void ANBCharacter::ExitRagdoll()
{
	if (!bIsRagdoll) return;
	if (CombatState == ECombatState::Dead) return;

	bIsRagdoll = false;
	CombatState = ECombatState::Idle;

	// Multicast exit ragdoll. Khong can truyen vi tri pelvis (khong teleport actor).
	Multicast_SetRagdoll(false, FVector::ZeroVector);

	if (GetUpMontage)
	{
		Multicast_PlayMontage(GetUpMontage);
	}
}

void ANBCharacter::Multicast_SetRagdoll_Implementation(bool bEnable, FVector InitialImpulse)
{
	USkeletalMeshComponent* SkelMesh = GetMesh();
	UCapsuleComponent* Cap = GetCapsuleComponent();
	if (!SkelMesh || !Cap) return;

	if (bEnable)
	{
		// Force detach victim khoi grabber TRUOC khi enable physics.
		// Tranh latency: ket qua client thay attached → bay qua → detach → desync.
		if (GetAttachParentActor())
		{
			DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
		}

		// ENTER ragdoll: enable physics, disable movement.
		if (UCharacterMovementComponent* Mv = GetCharacterMovement())
		{
			Mv->DisableMovement();
			Mv->StopMovementImmediately();
		}
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		SkelMesh->SetCollisionProfileName(RagdollCollisionProfile);
		SkelMesh->SetSimulatePhysics(true);
		SkelMesh->SetAllBodiesPhysicsBlendWeight(1.f);

		if (!InitialImpulse.IsNearlyZero())
		{
			// bVelChange=false: impulse acts as force/mass, light scaling.
			SkelMesh->AddImpulseToAllBodiesBelow(InitialImpulse, NAME_None, true, false);
		}
	}
	else
	{
		// EXIT ragdoll: snap mesh back to capsule, restore movement.
		// KHONG teleport actor - mesh se tu snap ve capsule (actor stay).

		// Force upright rotation cua actor.
		FRotator CurrentRot = GetActorRotation();
		FRotator UprightRot(0.f, CurrentRot.Yaw, 0.f);
		SetActorRotation(UprightRot, ETeleportType::TeleportPhysics);

		SkelMesh->SetSimulatePhysics(false);
		SkelMesh->SetCollisionProfileName(MeshDefaultCollisionProfile);

		// Snap mesh ve relative offset goc cua capsule.
		SkelMesh->AttachToComponent(Cap, FAttachmentTransformRules::KeepRelativeTransform);
		SkelMesh->SetRelativeLocation(MeshDefaultRelativeLocation);
		SkelMesh->SetRelativeRotation(MeshDefaultRelativeRotation);

		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (UCharacterMovementComponent* Mv = GetCharacterMovement())
		{
			Mv->SetMovementMode(MOVE_Falling);
		}
	}
}

// =========================================================
// Ultimate
// =========================================================

void ANBCharacter::TryUseUltimate()
{
	if (CombatState != ECombatState::Idle && CombatState != ECombatState::Grabbing) return;
	if (!IsUltimateReady()) return;
	if (!UltimateInstance->CanActivate(this)) return;

	if (CombatState == ECombatState::Grabbing) ReleaseGrab();
	StartUltimate();
}

void ANBCharacter::StartUltimate()
{
	CombatState = ECombatState::UsingUltimate;
	UltimateCooldownRemaining = UltimateInstance->CooldownSeconds;

	// Reset Rage khi dung Ultimate.
	RageGauge = 0.f;

	if (bHasAimYaw)
	{
		if (AController* C = GetController())
		{
			C->SetControlRotation(FRotator(0.f, DesiredAimYaw, 0.f));
		}
	}

	UltimateInstance->OnActivate(this);
}

void ANBCharacter::Notify_UltimateFinished()
{
	EndUltimate();
}

void ANBCharacter::EndUltimate()
{
	if (CombatState == ECombatState::UsingUltimate)
	{
		CombatState = ECombatState::Idle;
	}
}

void ANBCharacter::TickCooldowns(float DeltaSeconds)
{
	if (UltimateCooldownRemaining > 0.f)
	{
		UltimateCooldownRemaining = FMath::Max(0.f, UltimateCooldownRemaining - DeltaSeconds);
	}
}

// =========================================================
// Death
// =========================================================

void ANBCharacter::HandleDeath(AActor*)
{
	if (CombatState == ECombatState::Dead) return;

	if (GrabbedCharacter.IsValid()) ReleaseGrab();

	GetWorldTimerManager().ClearTimer(Timer_RagdollRecover);
	EnterRagdoll(FVector::ZeroVector);
	CombatState = ECombatState::Dead;
	GetWorldTimerManager().ClearTimer(Timer_RagdollRecover);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// Notify GameMode (server-only).
	if (HasAuthority())
	{
		if (ANBGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ANBGameMode>() : nullptr)
		{
			GM->OnCharacterDied(this);
		}
	}
}

// =========================================================
// Anim sync
// =========================================================

void ANBCharacter::UpdateAnimSync(float)
{
	const FVector Vel = GetVelocity();
	const FVector Planar(Vel.X, Vel.Y, 0.f);
	Speed = Planar.Size();

	// Tự tính direction angle (-180..180) bằng dot product.
	if (Speed > 1.f)
	{
		const FVector VelDir = Planar.GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), VelDir);
		const float RightDot = FVector::DotProduct(GetActorRightVector(), VelDir);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
	}
	else
	{
		Direction = 0.f;
	}
}

// =========================================================
// Server RPCs - client → server
// =========================================================

void ANBCharacter::Server_RequestAttack_Implementation()
{
	// Server-authoritative: chi server quyet dinh co attack hay khong.
	if (!CanAcceptInput()) return;

	UCharacterMovementComponent* Mv = GetCharacterMovement();
	if (CombatState == ECombatState::Grabbing)
	{
		TryThrow();
		return;
	}
	if (CombatState != ECombatState::Idle) return;

	if (Mv && Mv->IsFalling() && KickMontage)
	{
		StartKick();
	}
	else
	{
		StartAttack();
	}
}

void ANBCharacter::Server_RequestGrab_Implementation()
{
	if (!CanAcceptInput()) return;
	bGrabHeld = true;
	TryStartGrab();
}

void ANBCharacter::Server_RequestRelease_Implementation()
{
	bGrabHeld = false;
	if (CombatState == ECombatState::Grabbing)
	{
		ReleaseGrab();
	}
}

void ANBCharacter::Server_RequestUltimate_Implementation()
{
	if (!CanAcceptInput()) return;
	TryUseUltimate();
}

void ANBCharacter::Server_RequestJump_Implementation()
{
	if (!CanAcceptInput()) return;
	if (CombatState != ECombatState::Idle && CombatState != ECombatState::Grabbing) return;

	// Dung LaunchCharacter thay Jump() de replicate dung qua CharacterMovement.
	UCharacterMovementComponent* Mv = GetCharacterMovement();
	if (!Mv || Mv->IsFalling()) return;

	const FVector LaunchVel(0.f, 0.f, JumpZVelocity);
	LaunchCharacter(LaunchVel, false, true);
}

void ANBCharacter::Server_SetSprinting_Implementation(bool bSprint)
{
	bWantsToSprint = bSprint;
}

void ANBCharacter::Server_UpdateAim_Implementation(float NewYaw)
{
	DesiredAimYaw = NewYaw;
	bHasAimYaw = true;
}

// =========================================================
// Multicast RPCs - server → all clients
// =========================================================

void ANBCharacter::Multicast_PlayMontage_Implementation(UAnimMontage* Montage)
{
	if (!Montage) return;

	// CRITICAL: Skip owner client - owner da play montage local (client-side prediction).
	// Tranh play 2 lan (jitter).
	// Server (Listen Server) van play vi server cung la owner cua minh.
	if (IsLocallyControlled() && !HasAuthority())
	{
		return;
	}

	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		Anim->Montage_Play(Montage);
	}
}