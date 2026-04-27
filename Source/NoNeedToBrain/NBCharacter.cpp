#include "NBCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

#include "InputAction.h"
#include "InputMappingContext.h"

#include "Animation/AnimInstance.h"
#include "Kismet/KismetMathLibrary.h"

ANBCharacter::ANBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Strafing movement: rotate by controller, NOT by movement direction
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.f;
	SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}

void ANBCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enhanced Input mapping context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (IMC_Player)
				{
					Sub->AddMappingContext(IMC_Player, 0);
				}
			}
		}
	}
}

void ANBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Rotation system: ALWAYS runs, independent from attack
	UpdateAimSources(DeltaSeconds);
	UpdateControlRotation(DeltaSeconds);

	// Anim sync variables
	UpdateAnimSync(DeltaSeconds);
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
}

// =========================
// Input
// =========================

void ANBCharacter::Input_Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();

	if (!Controller) return;

	// Move relative to control yaw for strafing
	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	// Convention: X = Right, Y = Forward (common for stick/WASD bindings)
	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right, MoveInput.X);
}

void ANBCharacter::Input_Aim(const FInputActionValue& Value)
{
	const FVector2D Aim = Value.Get<FVector2D>();

	// Store the last meaningful stick aim
	if (Aim.SizeSquared() >= FMath::Square(AimStickDeadzone))
	{
		LastAimStick = Aim;
	}
}

void ANBCharacter::Input_Attack(const FInputActionValue& /*Value*/)
{
	TryStartAttack();
}

// =========================
// Rotation system
// =========================

void ANBCharacter::UpdateAimSources(float /*DeltaSeconds*/)
{
	// Determine desired yaw from aim direction (mouse preferred, else stick)
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
	if (!bHasAimYaw) return;

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
	if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		return false;
	}

	FVector Dir = Hit.ImpactPoint - GetActorLocation();
	Dir.Z = 0.f;

	if (Dir.IsNearlyZero())
	{
		return false;
	}

	OutWorldDir = Dir.GetSafeNormal();
	return true;
}

bool ANBCharacter::TryGetJoystickAimDirection(FVector& OutWorldDir) const
{
	// Convert 2D stick aim into a world direction in X/Y plane.
	// Using same convention as your previous code: world dir = (Y, -X, 0)
	if (LastAimStick.SizeSquared() < FMath::Square(AimStickDeadzone))
	{
		return false;
	}

	const FVector Dir(LastAimStick.Y, -LastAimStick.X, 0.f);
	if (Dir.IsNearlyZero())
	{
		return false;
	}

	OutWorldDir = Dir.GetSafeNormal();
	return true;
}

bool ANBCharacter::TryGetAimWorldDirection(FVector& OutWorldDir) const
{
	// Mouse preferred first (top-down)
	if (bMouseAimPreferred)
	{
		if (TryGetMouseAimDirection(OutWorldDir))
		{
			return true;
		}

		return TryGetJoystickAimDirection(OutWorldDir);
	}

	// Stick preferred first
	if (TryGetJoystickAimDirection(OutWorldDir))
	{
		return true;
	}
	return TryGetMouseAimDirection(OutWorldDir);
}

// =========================
// Attack system
// =========================

bool ANBCharacter::CanAttack() const
{
	return CombatState == ECombatState::Idle;
}

void ANBCharacter::TryStartAttack()
{
	if (!CanAttack())
	{
		return;
	}

	StartAttack();
}

void ANBCharacter::StartAttack()
{
	CombatState = ECombatState::Attacking;

	// Play montage if provided (optional)
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (AttackMontage)
		{
			Anim->Montage_Play(AttackMontage);
		}
	}

	// Simple lock using timer (no buffering)
	GetWorldTimerManager().ClearTimer(Timer_AttackLock);
	GetWorldTimerManager().SetTimer(
		Timer_AttackLock,
		this,
		&ANBCharacter::FinishAttack,
		AttackLockSeconds,
		false
	);
}

void ANBCharacter::FinishAttack()
{
	// Safety: avoid stuck state
	CombatState = ECombatState::Idle;
}

void ANBCharacter::Notify_AttackFinished()
{
	// If you later switch to AnimNotify, call this from AnimNotify and optionally disable the timer.
	FinishAttack();
}

// =========================
// Anim sync
// =========================

void ANBCharacter::UpdateAnimSync(float /*DeltaSeconds*/)
{
	const FVector Vel = GetVelocity();
	const FVector Planar(Vel.X, Vel.Y, 0.f);
	Speed = Planar.Size();

	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		Direction = Anim->CalculateDirection(Vel, GetActorRotation());
	}
	else
	{
		Direction = 0.f;
	}
}