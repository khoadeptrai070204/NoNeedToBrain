#include "NBCharacter.h"
#include "NBUltimateAbility.h"
#include "NBHealthComponent.h"

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
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

ANBCharacter::ANBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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

void ANBCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Set tốc độ đi bộ ban đầu (lần đầu tiên — Tick sẽ tự cập nhật sau).
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->MaxWalkSpeed = WalkSpeed;
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
	return UltimateInstance && UltimateCooldownRemaining <= 0.f;
}

FVector ANBCharacter::GetAimDirectionWorld() const
{
	if (bHasAimYaw) return FRotator(0.f, DesiredAimYaw, 0.f).Vector();
	return GetActorForwardVector();
}

// =========================================================
// Input
// =========================================================

void ANBCharacter::Input_Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (!Controller || !CanMove()) return;

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

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
	if (!CanAcceptInput()) return;

	if (CombatState == ECombatState::Grabbing)
	{
		TryThrow();
		return;
	}
	TryStartAttack();
}

void ANBCharacter::Input_GrabPressed(const FInputActionValue&)
{
	if (!CanAcceptInput()) return;
	bGrabHeld = true;
	TryStartGrab();
}

void ANBCharacter::Input_GrabReleased(const FInputActionValue&)
{
	bGrabHeld = false;
	if (CombatState == ECombatState::Grabbing)
	{
		ReleaseGrab();
	}
}

void ANBCharacter::Input_Ultimate(const FInputActionValue&)
{
	if (!CanAcceptInput()) return;
	TryUseUltimate();
}

void ANBCharacter::Input_SprintPressed(const FInputActionValue&)
{
	// Cứ set intent. Có sprint thật hay không sẽ do UpdateMovementSpeed quyết.
	bWantsToSprint = true;
}

void ANBCharacter::Input_SprintReleased(const FInputActionValue&)
{
	bWantsToSprint = false;
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

	if (bHasAimYaw)
	{
		if (AController* C = GetController())
		{
			C->SetControlRotation(FRotator(0.f, DesiredAimYaw, 0.f));
		}
	}

	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (AttackMontage) Anim->Montage_Play(AttackMontage);
	}

	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	GetWorldTimerManager().SetTimer(
		Timer_AttackLock, this, &ANBCharacter::EndAttack, AttackLockSeconds, false);
}

void ANBCharacter::EndAttack()
{
	bAttackHitWindowOpen = false;
	HitThisSwing.Reset();
	if (CombatState == ECombatState::Attacking)
	{
		CombatState = ECombatState::Idle;
	}
}

void ANBCharacter::Notify_AttackHitWindowStart()
{
	if (CombatState != ECombatState::Attacking) return;
	bAttackHitWindowOpen = true;
	HitThisSwing.Reset();
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

	const FVector Start = GetActorLocation() + GetActorForwardVector() * AttackHitRange * 0.5f;
	FCollisionShape Shape = FCollisionShape::MakeSphere(AttackHitRadius);
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
			if (Other->HealthComp)
			{
				Other->HealthComp->ApplyDamage(AttackDamage, this);
			}

			FVector Push = Other->GetActorLocation() - GetActorLocation();
			Push.Z = 0.f;
			Push = Push.GetSafeNormal();
			const FVector Impulse = Push * AttackKnockbackImpulse + FVector::UpVector * AttackUpwardImpulse;

			if (UCharacterMovementComponent* Mv = Other->GetCharacterMovement())
			{
				Mv->AddImpulse(Impulse, true);
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
	if (CombatState != ECombatState::Idle) return;

	ANBCharacter* Target = nullptr;
	if (FindGrabTarget(Target) && Target)
	{
		StartGrab(Target);
	}
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

	CombatState = ECombatState::Grabbing;
	GrabbedCharacter = Target;
	Target->OnGrabbedBy(this);

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);
	Target->AttachToComponent(GetMesh(), Rules, GrabSocketName);

	// Speed sẽ tự update trong Tick → UpdateMovementSpeed.

	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (GrabMontage) Anim->Montage_Play(GrabMontage);
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

	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (ThrowMontage) Anim->Montage_Play(ThrowMontage);
	}

	GetWorldTimerManager().ClearTimer(Timer_ThrowLock);
	GetWorldTimerManager().SetTimer(
		Timer_ThrowLock, this, &ANBCharacter::EndThrow, ThrowLockSeconds, false);
}

void ANBCharacter::Notify_ThrowRelease()
{
	if (CombatState != ECombatState::Throwing) return;
	if (!GrabbedCharacter.IsValid()) return;

	ANBCharacter* V = GrabbedCharacter.Get();
	V->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));

	const FVector Dir = GetAimDirectionWorld();
	const FVector Impulse = Dir * ThrowImpulse + FVector::UpVector * ThrowUpwardImpulse;

	V->OnReleasedBy(this, true, Impulse);
	GrabbedCharacter = nullptr;
}

void ANBCharacter::Notify_ThrowFinished()
{
	GetWorldTimerManager().ClearTimer(Timer_ThrowLock);
	EndThrow();
}

void ANBCharacter::EndThrow()
{
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

	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->DisableMovement();
		Mv->StopMovementImmediately();
	}
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ANBCharacter::OnReleasedBy(ANBCharacter*, bool bThrown, FVector ThrowImpulseVec)
{
	bIsGrabbed = false;
	Grabber = nullptr;

	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->SetMovementMode(MOVE_Falling);
	}

	if (bThrown)
	{
		EnterRagdoll(ThrowImpulseVec);
	}
	else
	{
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

	if (GrabbedCharacter.IsValid())
	{
		ReleaseGrab();
	}

	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->DisableMovement();
		Mv->StopMovementImmediately();
	}
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionProfileName(RagdollCollisionProfile);
		SkelMesh->SetSimulatePhysics(true);
		SkelMesh->SetAllBodiesPhysicsBlendWeight(1.f);

		if (!InitialImpulse.IsNearlyZero())
		{
			SkelMesh->AddImpulse(InitialImpulse, NAME_None, true);
		}
	}

	GetWorldTimerManager().ClearTimer(Timer_RagdollRecover);
	GetWorldTimerManager().SetTimer(
		Timer_RagdollRecover, this, &ANBCharacter::ExitRagdoll, RagdollDuration, false);
}

void ANBCharacter::ExitRagdoll()
{
	if (!bIsRagdoll) return;
	if (CombatState == ECombatState::Dead) return;

	USkeletalMeshComponent* SkelMesh = GetMesh();
	UCapsuleComponent* Cap = GetCapsuleComponent();
	if (!SkelMesh || !Cap) return;

	const FVector PelvisLoc = SkelMesh->GetSocketLocation(RagdollPelvisBone);

	SkelMesh->SetSimulatePhysics(false);
	SkelMesh->SetCollisionProfileName(MeshDefaultCollisionProfile);

	SkelMesh->AttachToComponent(Cap, FAttachmentTransformRules::KeepRelativeTransform);
	SkelMesh->SetRelativeLocation(MeshDefaultRelativeLocation);
	SkelMesh->SetRelativeRotation(MeshDefaultRelativeRotation);

	const float CapHalf = Cap->GetScaledCapsuleHalfHeight();
	SetActorLocation(PelvisLoc + FVector(0.f, 0.f, CapHalf), false, nullptr, ETeleportType::TeleportPhysics);

	Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->SetMovementMode(MOVE_Falling);
	}

	if (GetUpMontage)
	{
		if (UAnimInstance* Anim = SkelMesh->GetAnimInstance())
		{
			Anim->Montage_Play(GetUpMontage);
		}
	}

	bIsRagdoll = false;
	CombatState = ECombatState::Idle;
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
	// Forward dot → thành phần "trước/sau", Right dot → thành phần "trái/phải".
	// Atan2(Right, Forward) cho ra góc tiêu chuẩn cho BlendSpace Direction.
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