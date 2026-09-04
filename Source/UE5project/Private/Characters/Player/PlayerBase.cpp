// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Player/PlayerBase.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Engine/OverlapResult.h"

// 이동
#include "Characters/Components/BaseCharacterMovementComponent.h"

// 콜리전
#include "Components/CapsuleComponent.h"

// 카메라
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

// 입력
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "InputMappingContext.h"
#include "Characters/Player/InputConfigDataAsset.h"

// Kismet 유틸리티
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet/KismetMathLibrary.h"

// 애니메이션
#include "Characters/Player/PlayerBaseAnimInstance.h"

// 참조할 액터
#include "Characters/Player/PlayerRide.h"

// 인터페이스
#include "Interaction/Interfaces/InteractInterface.h"

// 유저 컴포넌트
#include "Characters/Player/Components/PlayerStatusComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Components/RideComponent.h"
#include "Characters/Player/Components/InventoryComponent.h" 
#include "Combat/Components/CombatComponent.h"
#include "Combat/Components/PlayerAttackComponent.h"
#include "Combat/Components/PlayerHitReactionComponent.h"
#include "Interaction/Components/InteractComponent.h"
#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Characters/Player/Components/LockOnComponent.h"

// 데이터 참조
#include "Characters/Player/PlayerConfig.h"
#include "Core/Subsystems/GameInstanceSystem/PlayerAnimRegistrySubsystem.h"

// 유틸리티
#include "Utils/GameplayTagsBase.h"
#include "Utils/CoreLog.h"

/* ============================================================
 *  Constructor
 * ============================================================ */
APlayerBase::APlayerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
	.SetDefaultSubobjectClass<UPlayerAttackComponent>(TEXT("AttackComponent"))
	.SetDefaultSubobjectClass<UPlayerHitReactionComponent>(TEXT("HitReactionComponent"))
	.SetDefaultSubobjectClass<UPlayerStatusComponent>(TEXT("CharacterStatusComponent"))
	.SetDefaultSubobjectClass<UPlayerStatComponent>(TEXT("StatComponent")))
{
	PrimaryActorTick.bCanEverTick = true;

	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->bAutoActivate = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->bAutoActivate = true;

	InteractComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("InteractComponent"));
	InteractComponent->bAutoActivate = true;

	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));
	LockOnComponent->bAutoActivate = true;

	RideComponent = CreateDefaultSubobject<URideComponent>(TEXT("RideComponent"));
	RideComponent->bAutoActivate = true;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Character_Player"));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	GetCapsuleComponent()->SetCapsuleHalfHeight(90.0f);

	GetMesh()->SetGenerateOverlapEvents(true);

	CurLocomotionGait = ELocomotionGait::Jog;

	GetCharacterMovement()->MaxWalkSpeed = 450.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 800.0f;
	GetCharacterMovement()->MaxAcceleration = 800.0f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;
	GetCharacterMovement()->JumpZVelocity = 500.0f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->GravityScale = 1.2f;
	GetCharacterMovement()->GroundFriction = 10.0f;

	EquipmentComponent->SetWeaponSocketName(FName("S_Sword"));
	EquipmentComponent->SetSubEquipSocketName(FName("S_SubEquip"));

	CameraSetting();
	CurrentProfileTag = FGameplayTag::RequestGameplayTag(FName("Skeleton.Player"));
	Tags.Add("Player");
}

/* ============================================================
 *  BeginPlay
 * ============================================================ */
void APlayerBase::BeginPlay()
{
	Super::BeginPlay();

	ApplyConfig();
	RefreshActionAnimationProfile(EWeaponType::None);

	CharacterBaseAnim = Cast<UPlayerBaseAnimInstance>(GetMesh()->GetAnimInstance());

	if (CharacterBaseAnim)
	{
	}

	ActivateGroundInputContext();

	InitSpringArmLocation = SpringArm->GetRelativeLocation();
}

void APlayerBase::ActivateGroundInputContext()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !InputConfig) return;
	if (UEnhancedInputLocalPlayerSubsystem* SubSystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		if (InputConfig->RideContext) SubSystem->RemoveMappingContext(InputConfig->RideContext);
		if (InputConfig->DefaultContext) SubSystem->AddMappingContext(InputConfig->DefaultContext, 0);
	}
}

void APlayerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 실행자가 제거되더라도 피처형 대상이 Execution/AI 정지 상태에 남지 않게 한다.
	if (ActiveCriticalExecutionTarget.IsValid() || ActiveCriticalExecutionMontage)
	{
		ExitCriticalExecutionRuntime(EActionExitReason::Interrupted);
	}
	Super::EndPlay(EndPlayReason);
}

/* ============================================================
 *  Tick
 * ============================================================ */
void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 동기 Execution 도중 대상이 외부 사망/제거되면 몽타주 종료까지 실행자를
	// 묶어 두지 않는다. 정상 Execution 완료로 발생한 사망은 DeathPending으로 구분한다.
	if (ActiveCriticalExecutionMontage)
	{
		AEnemyBase* ExecutionTarget = ActiveCriticalExecutionTarget.Get();
		const bool bTargetUnavailable = !ExecutionTarget;
		const bool bUnexpectedTargetDeath = ExecutionTarget &&
			ExecutionTarget->GetCharacterStatusComponent()->IsDead() &&
			!ExecutionTarget->IsCriticalExecutionDeathPending();
		if (bTargetUnavailable || bUnexpectedTargetDeath)
		{
			ExitCriticalExecutionRuntime(EActionExitReason::Interrupted);
			FinishActionIfCurrent(TAG_Action_Execution);
		}
	}

	GuardReentryLockoutRemaining = FMath::Max(0.f, GuardReentryLockoutRemaining - DeltaTime);
	if (bForcedRotatingInputDirection)
	{
		FRotator CurrentRot = GetActorRotation();
		FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, InputRotation, DeltaTime, ForcedRotationSpeed);

		if (FMath::Abs((NewRot - InputRotation).Yaw) < 1.0f)
		{
			SetActorRotation(InputRotation);
			bForcedRotatingInputDirection = false;
		}
		else
		{
			SetActorRotation(NewRot);
		}
	}

	if (LockOnComponent && LockOnComponent->IsLockedOn())
	{
		LockOnComponent->TickLockOn(DeltaTime);
	}

	UpdateChargeAttack(DeltaTime);
	UpdateDodgeSprintInput(DeltaTime);

	if (GetStatComponent())
	{
		if (CurLocomotionGait == ELocomotionGait::Sprint && GetVelocity().SizeSquared() > 100.f)
		{
			GetStatComponent()->ChangeStamina(SprintStaminaPerSec * DeltaTime, EStatChangeType::Damage);
			if (GetStatComponent()->GetStamina() <= 0.f)
				Jog();   // 바닥나면 조그로 강제 전환
		}

		const bool bIsActivelyGuarding = GetCharacterStatusComponent() &&
			GetCharacterStatusComponent()->GetCurrentAction().MatchesTagExact(TAG_Action_Guard);
		GetStatComponent()->TickStaminaRegen(
			DeltaTime, bIsActivelyGuarding ? GuardStaminaRegenMultiplier : 1.0f);
	}


	// 디버그 드로잉 (기존 유지)
	FVector LastInputDirection = GetLastMovementInputVector().GetSafeNormal();
	if (!LastInputDirection.IsNearlyZero())
	{
		FVector MovementDirection = GetVelocity().GetSafeNormal();
		FVector DebugStartLocation = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		float DotProduct = FVector::DotProduct(MovementDirection, LastInputDirection);
		float RadianAngle = FMath::Acos(DotProduct);
		float DegreeAngle = FMath::RadiansToDegrees(RadianAngle);

		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1);

		FText DebugAxisText = FText::AsNumber(DegreeAngle, &FormatOptions);
		FString DebugAxisString = DebugAxisText.ToString();

		float DebugLineLength = 100.0f;

		DrawDebugDirectionalArrow(GetWorld(), DebugStartLocation,
			DebugStartLocation + MovementDirection * DebugLineLength,
			50.0f, FColor::Green, false, 0.0f, 0, 2.0f);

		DrawDebugDirectionalArrow(GetWorld(), DebugStartLocation,
			DebugStartLocation + LastInputDirection * DebugLineLength,
			50.0f, FColor::Blue, false, 0.0f, 0, 2.0f);

		DrawDebugString(GetWorld(), DebugStartLocation, DebugAxisString,
			0, FColor::White, 0.0f);
	}
}

/* ============================================================
 *  ApplyConfig
 * ============================================================ */
void APlayerBase::ApplyConfig()
{
	if (!Config) { ensureMsgf(false, TEXT("Config missing")); return; }

	GetCharacterStatusComponent()->SetWindowRules(Config->WindowRules);
	GetMesh()->SetSkeletalMesh(Config->Mesh);
	GetMesh()->SetAnimInstanceClass(Config->AnimBP);
	GetHitReactionComponent()->SetHitReactionListDA(Config->HitReactData);
	GetAttackComponent()->SetAttackDA(Config->AttackData);
}

/* ============================================================
 *  Camera
 * ============================================================ */
void APlayerBase::CameraSetting()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));

	SpringArm->SetupAttachment(GetCapsuleComponent());
	Camera->SetupAttachment(SpringArm);
	Camera->SetConstraintAspectRatio(false);

	SpringArm->TargetArmLength = 200.0f;
	SpringArm->SocketOffset = FVector(0.0f, 60.0f, 0.0f);
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bDoCollisionTest = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f);
}

/* ============================================================
 *  Input Binding
 * ============================================================ */
void APlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InputConfig->Move, ETriggerEvent::Triggered, this, &APlayerBase::Move);
		EnhancedInputComponent->BindAction(InputConfig->CheckMove, ETriggerEvent::Started, this, &APlayerBase::StartMoveInput);
		EnhancedInputComponent->BindAction(InputConfig->CheckMove, ETriggerEvent::Completed, this, &APlayerBase::EndMoveInput);

		EnhancedInputComponent->BindAction(InputConfig->Look, ETriggerEvent::Triggered, this, &APlayerBase::Look);

		// ★ 변경: 커브 체크 → RequestAction 방식
		EnhancedInputComponent->BindAction(InputConfig->Jump, ETriggerEvent::Triggered, this, &APlayerBase::JumpInput);
		EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Started, this, &APlayerBase::DodgeSprintInputStarted);
		EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Completed, this, &APlayerBase::DodgeSprintInputCompleted);
		EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Canceled, this, &APlayerBase::DodgeSprintInputCanceled);

		EnhancedInputComponent->BindAction(InputConfig->Block, ETriggerEvent::Ongoing, this, &APlayerBase::BlockInput);
		EnhancedInputComponent->BindAction(InputConfig->Block, ETriggerEvent::Triggered, this, &APlayerBase::BlockInputEnd);

		if (InputConfig->Parry)
		{
			EnhancedInputComponent->BindAction(InputConfig->Parry, ETriggerEvent::Started, this, &APlayerBase::ParryInput);
		}

		EnhancedInputComponent->BindAction(InputConfig->Interact, ETriggerEvent::Triggered, this, &APlayerBase::InteractInput);

		EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Started, this, &APlayerBase::Walk);
		EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Triggered, this, &APlayerBase::Jog);

		EnhancedInputComponent->BindAction(InputConfig->SpawnRide, ETriggerEvent::Started, this, &APlayerBase::SpawnRideInput);
		EnhancedInputComponent->BindAction(InputConfig->SpawnRide, ETriggerEvent::Completed, this, &APlayerBase::SpawnRideInputCompleted);

		EnhancedInputComponent->BindAction(InputConfig->Attack, ETriggerEvent::Started, this, &APlayerBase::AttackInputStarted);
		EnhancedInputComponent->BindAction(InputConfig->Attack, ETriggerEvent::Completed, this, &APlayerBase::AttackInputCompleted);
		EnhancedInputComponent->BindAction(InputConfig->Attack, ETriggerEvent::Canceled, this, &APlayerBase::AttackInputCanceled);

		EnhancedInputComponent->BindAction(InputConfig->Modifier, ETriggerEvent::Started, this, &APlayerBase::ModifierInput);
		EnhancedInputComponent->BindAction(InputConfig->Modifier, ETriggerEvent::Completed, this, &APlayerBase::ModifierInputEnd);
		EnhancedInputComponent->BindAction(InputConfig->Modifier, ETriggerEvent::Canceled, this, &APlayerBase::ModifierInputEnd);

		EnhancedInputComponent->BindAction(InputConfig->LockOn, ETriggerEvent::Triggered, this, &APlayerBase::OnLockOnToggle);
		EnhancedInputComponent->BindAction(InputConfig->LockOnSwitchLeft, ETriggerEvent::Triggered, this, &APlayerBase::OnLockOnSwitchLeft);
		EnhancedInputComponent->BindAction(InputConfig->LockOnSwitchRight, ETriggerEvent::Triggered, this, &APlayerBase::OnLockOnSwitchRight);
	}
}

/* ============================================================
 *  PostInitializeComponents — 델리게이트 바인딩
 * ============================================================ */
void APlayerBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InteractComponent->OnArrivedInteractionPoint.BindUObject(this, &APlayerBase::HandleArrivedInteractionPoint);
	InteractComponent->OnInteractionMoveCancelled.BindUObject(this, &APlayerBase::HandleInteractionMoveCancelled);
	EquipmentComponent->OnWeaponChangedDelegate.AddUObject(this, &APlayerBase::RefreshActionAnimationProfile);
	LockOnComponent->OnLockOnTargetChanged.AddUObject(this, &APlayerBase::HandleLockOnTargetChanged);

	if (GetCharacterStatusComponent())
	{
		// ★ 버퍼에서 소비된 행동의 실제 실행을 위한 바인딩
		GetCharacterStatusComponent()->OnActionConsumed.BindUObject(this, &APlayerBase::HandleBufferedAction);
		GetCharacterStatusComponent()->OnActionTransition.BindUObject(this, &APlayerBase::HandleActionTransition);

		if (GetAttackComponent())
		{
			GetAttackComponent()->OnAttackFinished.AddUObject(this, &APlayerBase::HandleAttackFinished);
			GetAttackComponent()->OnComboAttackRequested.BindUObject(this, &APlayerBase::HandleComboAttackRequested);
		}
		if(GetClimbComponent()) GetClimbComponent()->OnLadderExit.AddUObject(this, &APlayerBase::HandleLadderExit);
	}

	if(GetStatComponent()) GetStatComponent()->InitializeStats();
}

/* ============================================================
 *  버퍼 소비 콜백 — 버퍼링된 입력이 Window 열릴 때 자동 실행
 * ============================================================ */
void APlayerBase::HandleBufferedAction(const FGameplayTag& ActionTag)
{
	if (ActionTag == TAG_Action_Attack)
	{
		ExecuteAttack();
	}
	else if (ActionTag == TAG_Action_Jump)
	{
		ExecuteJump();
	}
	else if (ActionTag == TAG_Action_Dodge)
	{
		ExecuteDodge();
	}
	else if (ActionTag == TAG_Action_Guard)
	{
		if (bWantsToGuard)
		{
			ExecuteBlock();
		}
		else
		{
			FinishActionIfCurrent(TAG_Action_Guard);
		}
	}
	else if (ActionTag == TAG_Action_Parry)
	{
		ExecuteParry();
	}
	else if (ActionTag == TAG_Action_Interact)
	{
		ExecuteInteract();
	}
}

/* ============================================================
 *  Move / Look
 * ============================================================ */
void APlayerBase::Move(const FInputActionValue& value)
{
	IsMovementInput = true;

	const FVector2D DirectionValue = value.Get<FVector2D>();

	if(GetCharacterStatusComponent()->GetCurrentState() == TAG_State_Ground)
	{
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		InputVector = FVector(DirectionValue.X, DirectionValue.Y, 0.0f);
		TryReturnToLocomotion(DirectionValue);
		if (GetCharacterStatusComponent()->GetCurrentAction().MatchesTagExact(TAG_Action_HitReact) &&
			!GetCharacterStatusComponent()->IsWindowOpen(TAG_Window_Locomotion))
		{
			return;
		}

		FVector2D MovementScale = DirectionValue;
		MovementScale.Normalize();

		if (LockOnComponent->IsLockedOn())
		{
			float ForwardScale = CurLocomotionGait == ELocomotionGait::Sprint ? 1.0f : 0.8f;
			float BackwardScale = 0.85f;

			MovementScale.Y *= MovementScale.Y > 0.0f ? ForwardScale : BackwardScale;
		}

		AddMovementInput(UKismetMathLibrary::GetForwardVector(YawRotation), MovementScale.Y);
		AddMovementInput(UKismetMathLibrary::GetRightVector(YawRotation), MovementScale.X);
	}
	else if(GetCharacterStatusComponent()->GetCurrentState() == TAG_State_Ladder)
	{
		IsMovementInput = FMath::IsNearlyZero(DirectionValue.Y) ? false : true;
		if (!IsMovementInput) return;

		DirectionValue.Y > 0.0f ? GetClimbComponent()->ClimbUpLadder() : GetClimbComponent()->ClimbDownLadder();
	}
}

void APlayerBase::Look(const FInputActionValue& value)
{
	const FVector2D LookAxisValue = value.Get<FVector2D>();
	AddControllerPitchInput(LookAxisValue.Y * 0.5f);
	AddControllerYawInput(LookAxisValue.X * -0.5f);
}

void APlayerBase::StartMoveInput()
{
	IsMovementInput = true;
}

void APlayerBase::EndMoveInput()
{
	IsMovementInput = false;
	if (IsValid(ClimbComponent))
	{
		ClimbComponent->ClearRepeatedClimbInput();
	}
}

/* ============================================================
 *  Combat Input — 판단부 (RequestAction으로 체크/버퍼)
 * ============================================================ */
void APlayerBase::AttackInputStarted()
{
	bAttackButtonHeld = true;
	const FName RequestedAttackName = bCombatModifierHeld
		? FName(TEXT("HeavyAttack")) : FName(TEXT("DefaultCombo"));
	if (!bCombatModifierHeld && TryStartCriticalExecution())
	{
		PendingAttackName = NAME_None;
		return;
	}

	if (GetAttackComponent() && GetCharacterStatusComponent() &&
		GetAttackComponent()->TryHandleComboInput(
			RequestedAttackName, GetCharacterStatusComponent()->BufferDuration))
	{
		GetCharacterStatusComponent()->RemoveBufferedAction(TAG_Action_Attack);
		PendingAttackName = NAME_None;
		return;
	}

	if (GetAttackComponent()) GetAttackComponent()->ClearBufferedComboInput();
	PendingAttackName = RequestedAttackName;

	const FBaseAttackData* NextAttack = GetAttackComponent()
		? GetAttackComponent()->GetNextAttackData(PendingAttackName) : nullptr;
	if (!NextAttack)
	{
		PendingAttackName = NAME_None;
		return;
	}
	const FAttackModifiers InitialModifiers = NextAttack->ResolveModifiers(0.0f);
	if (!GetStatComponent()->CanAffordStamina(GetAttackStaminaCost(PendingAttackName, &InitialModifiers)))
	{
		PendingAttackName = NAME_None;
		return;
	}

	if (GetCharacterStatusComponent()->RequestAction(TAG_Action_Attack))
	{
		ExecuteAttack(); // 즉시 가능하면 바로 실행
	}
	// else: 버퍼에 저장됨 → Window 열리면 HandleBufferedAction → ExecuteAttack
}

void APlayerBase::HandleComboAttackRequested(FName AttackName)
{
	UPlayerAttackComponent* PlayerAttack = GetAttackComponent();
	const FBaseAttackData* NextCombo = PlayerAttack
		? PlayerAttack->GetNextComboAttackData(AttackName) : nullptr;
	if (!NextCombo || !GetStatComponent()) return;

	const FAttackModifiers InitialModifiers = NextCombo->ResolveModifiers(0.0f);
	if (!GetStatComponent()->CanAffordStamina(GetAttackStaminaCost(AttackName, &InitialModifiers)))
	{
		return;
	}

	PendingAttackName = AttackName;
	ExecuteAttack();
}

void APlayerBase::AttackInputCompleted()
{
	bAttackButtonHeld = false;
	IsAttackInput = false;
	if (ChargeAttackPhase != EChargeAttackPhase::None)
	{
		RequestChargeTransition(EChargeTransitionRequest::Attack);
	}
}

void APlayerBase::AttackInputCanceled()
{
	bAttackButtonHeld = false;
	IsAttackInput = false;
	if (ChargeAttackPhase != EChargeAttackPhase::None)
	{
		RequestChargeTransition(EChargeTransitionRequest::End);
	}
	else if (PendingAttackName != NAME_None && GetCharacterStatusComponent())
	{
		GetCharacterStatusComponent()->RemoveBufferedAction(TAG_Action_Attack);
		PendingAttackName = NAME_None;
	}
}

void APlayerBase::JumpInput()
{
	if (GetCharacterStatusComponent()->RequestAction(TAG_Action_Jump))
	{
		ExecuteJump();
	}
}

void APlayerBase::DodgeInput()
{
	if (!GetStatComponent()->CanAffordStamina(GetDodgeStaminaCost())) return;

	if (GetCharacterStatusComponent()->RequestAction(TAG_Action_Dodge))
	{
		ExecuteDodge();
	}
}

void APlayerBase::BlockInput()
{
	bWantsToGuard = true;
	if (GuardReentryLockoutRemaining > 0.f) return;
	if (IsBlockInput) return;
	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	if (!StatusComponent) return;
	if (StatusComponent->RequestAction(TAG_Action_Guard))
	{
		ExecuteBlock();
	}
}

void APlayerBase::DodgeSprintInputStarted()
{
	bDodgeSprintInputHeld = true;
	bSprintStartedByHold = false;
	DodgeSprintHeldTime = 0.0f;
}

void APlayerBase::DodgeSprintInputCompleted()
{
	const bool bWasSprinting = bSprintStartedByHold;
	bDodgeSprintInputHeld = false;
	bSprintStartedByHold = false;
	DodgeSprintHeldTime = 0.0f;
	if (bWasSprinting)
	{
		Jog();
	}
	else
	{
		DodgeInput();
	}
}

void APlayerBase::DodgeSprintInputCanceled()
{
	bDodgeSprintInputHeld = false;
	DodgeSprintHeldTime = 0.0f;
	if (bSprintStartedByHold)
	{
		Jog();
	}
	bSprintStartedByHold = false;
}

void APlayerBase::UpdateDodgeSprintInput(float DeltaTime)
{
	if (!bDodgeSprintInputHeld || bSprintStartedByHold) return;
	DodgeSprintHeldTime += FMath::Max(0.0f, DeltaTime);
	if (DodgeSprintHeldTime >= SprintHoldThreshold)
	{
		bSprintStartedByHold = true;
		Sprint();
	}
}

bool APlayerBase::TryStartCriticalExecution()
{
	// 공격 입력이 일반 공격과 Execution 시도를 함께 담당하므로, 이미 Execution이
	// 진행 중일 때는 재탐색/몽타주 재생을 시도하지 않고 입력만 소비한다.
	// 여기서 false를 반환하면 AttackInput이 일반 공격 요청까지 계속 진행한다.
	if (IsCriticalExecutionActive())
	{
		return true;
	}

	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	if (!Config || !StatusComponent || !EquipmentComponent || !CharacterBaseAnim ||
		!StatusComponent->CanTryAction(TAG_Action_Execution))
	{
		return false;
	}
	const FCriticalExecutionSettings& ExecutionSettings = Config->CriticalExecution;

	auto IsEligibleTarget = [this, &ExecutionSettings](AEnemyBase* Enemy) -> bool
	{
		if (!Enemy || !Enemy->CanReceiveCriticalExecution(this))
		{
			return false;
		}

		const FVector ToEnemy = Enemy->GetActorLocation() - GetActorLocation();
		if (ToEnemy.SizeSquared2D() > FMath::Square(ExecutionSettings.SearchRange))
		{
			return false;
		}

		// 거리 안에 있더라도 월드 지형에 가려진 대상은 처형 대상으로 선택하지 않는다.
		FHitResult SightHit;
		FCollisionQueryParams SightQueryParams(SCENE_QUERY_STAT(CriticalExecutionSight), false, this);
		const bool bSightBlocked = GetWorld()->LineTraceSingleByChannel(
			SightHit, GetActorLocation(), Enemy->GetActorLocation(), ECC_Visibility, SightQueryParams);
		if (bSightBlocked && SightHit.GetActor() != Enemy)
		{
			return false;
		}

		const FVector Direction = ToEnemy.GetSafeNormal2D();
		const float MinDot = FMath::Cos(FMath::DegreesToRadians(ExecutionSettings.SearchMaxAngle));
		return FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), Direction) >= MinDot;
	};

	AEnemyBase* BestTarget = nullptr;
	if (LockOnComponent)
	{
		AEnemyBase* LockedEnemy = Cast<AEnemyBase>(LockOnComponent->GetCurrentTarget());
		if (IsEligibleTarget(LockedEnemy))
		{
			BestTarget = LockedEnemy;
		}
	}

	if (!BestTarget)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionObjectQueryParams ObjectQuery;
		ObjectQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CriticalExecutionSearch), false, this);
		GetWorld()->OverlapMultiByObjectType(
			Overlaps, GetActorLocation(), FQuat::Identity, ObjectQuery,
			FCollisionShape::MakeSphere(ExecutionSettings.SearchRange), QueryParams);

		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AEnemyBase* Enemy = Cast<AEnemyBase>(Overlap.GetActor());
			if (!IsEligibleTarget(Enemy))
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared2D(
				GetActorLocation(), Enemy->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestTarget = Enemy;
			}
		}
	}

	if (!BestTarget)
	{
		return false;
	}

	StatusComponent->SwitchAction(TAG_Action_Execution, EActionExitReason::Interrupted);
	UAnimMontage* AttackerMontage = nullptr;
	if (!BestTarget->BeginCriticalExecution(this, AttackerMontage) ||
		!AttackerMontage || CharacterBaseAnim->Montage_Play(AttackerMontage) <= 0.0f)
	{
		BestTarget->FinishCriticalExecution(this, false);
		FinishActionIfCurrent(TAG_Action_Execution);
		return false;
	}

	ActiveCriticalExecutionTarget = BestTarget;
	ActiveCriticalExecutionMontage = AttackerMontage;
	GetCharacterMovement()->DisableMovement();

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &APlayerBase::OnCriticalExecutionMontageEnded);
	CharacterBaseAnim->Montage_SetEndDelegate(EndDelegate, AttackerMontage);
	return true;
}

void APlayerBase::BlockInputEnd()
{
	bWantsToGuard = false;
	IsBlockInput = false;
	if (UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent())
	{
		StatusComponent->RemoveBufferedAction(TAG_Action_Guard);
	}
	FinishActionIfCurrent(TAG_Action_Guard);
}

void APlayerBase::ParryInput()
{
	if (UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent())
	{
		if (StatusComponent->RequestAction(TAG_Action_Parry))
		{
			ExecuteParry();
		}
	}
}

void APlayerBase::InteractInput()
{
	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	if (StatusComponent && StatusComponent->RequestAction(TAG_Action_Interact))
	{
		ExecuteInteract();
	}
}

void APlayerBase::SpawnRideInput()
{
	if (IsCriticalExecutionActive()) return;

	if (RideComponent)
	{
		RideComponent->HandleRideInputStarted();
	}
}

void APlayerBase::SpawnRideInputCompleted()
{
	if (RideComponent)
	{
		RideComponent->HandleRideInputCompleted();
	}
}

/* ============================================================
 *  Combat Execute — 실행부 (순수 로직, 판단 없음)
 * ============================================================ */
float APlayerBase::GetAttackStaminaCost(FName AttackName, const FAttackModifiers* Modifiers) const
{
	const UPlayerAttackComponent* PlayerAttack = GetAttackComponent();
	const FWeaponSetsInfo* Weapon = GetEquipmentComponent()
		? GetEquipmentComponent()->GetEquipedWeapon() : nullptr;
	const FBaseAttackData* NextAttack = PlayerAttack
		? PlayerAttack->GetNextAttackData(AttackName) : nullptr;
	const FAttackModifiers Resolved = Modifiers ? *Modifiers
		: (NextAttack ? NextAttack->ResolveModifiers() : FAttackModifiers{});
	return Weapon && NextAttack
		? FMath::Max(0.f, Weapon->StaminaCost * Resolved.StaminaCost)
		: 0.f;
}

float APlayerBase::GetDodgeStaminaCost() const
{
	const UPlayerStatComponent* PlayerStat = GetStatComponent();
	if (!PlayerStat)
	{
		return FMath::Max(0.f, DodgeStaminaBase);
	}

	const FPlayerStats Stats = PlayerStat->GetCharacterStats();
	const float LoadRatio = Stats.EquipLoad.Max > KINDA_SMALL_NUMBER
		? FMath::Clamp(Stats.EquipLoad.Current / Stats.EquipLoad.Max, 0.f, 1.f) : 0.f;
	return FMath::Max(0.f, DodgeStaminaBase * (1.0f + LoadRatio));
}

void APlayerBase::ExecuteAttack()
{
	const FName AttackName = PendingAttackName;
	PendingAttackName = NAME_None;
	if (AttackName.IsNone())
	{
		FinishActionIfCurrent(TAG_Action_Attack);
		return;
	}

	const FBaseAttackData* NextAttack = GetAttackComponent()->GetNextAttackData(AttackName);
	if (!NextAttack)
	{
		FinishActionIfCurrent(TAG_Action_Attack);
		return;
	}

	if (NextAttack->bCanCharge)
	{
		const FAttackModifiers MinimumModifiers = NextAttack->ResolveModifiers(0.0f);
		const float MinimumCost = GetAttackStaminaCost(AttackName, &MinimumModifiers);
		if (!GetStatComponent()->CanAffordStamina(MinimumCost))
		{
			FinishActionIfCurrent(TAG_Action_Attack);
			return;
		}

		const FBaseAttackData* Prepared = GetAttackComponent()->BeginChargeAttack(AttackName);
		if (!Prepared)
		{
			FinishActionIfCurrent(TAG_Action_Attack);
			return;
		}

		IsAttackInput = true;
		ActiveChargeAttackName = AttackName;
		ActiveChargeAttackData = *Prepared;
		ChargeAttackPhase = EChargeAttackPhase::Begin;
		ChargeTransitionRequest = bAttackButtonHeld
			? EChargeTransitionRequest::None : EChargeTransitionRequest::Attack;
		ChargeElapsed = 0.0f;
		return;
	}

	const FAttackModifiers AttackModifiers = NextAttack->ResolveModifiers();
	const float Cost = GetAttackStaminaCost(AttackName, &AttackModifiers);
	if (!GetStatComponent()->CanAffordStamina(Cost))
	{
		IsAttackInput = false;
		FinishActionIfCurrent(TAG_Action_Attack);
		return;
	}

	IsAttackInput = true;
	const FBaseAttackData* Played = GetAttackComponent()->ExecuteAttack(AttackName);
	if (!Played)
	{
		IsAttackInput = false;
		FinishActionIfCurrent(TAG_Action_Attack);
		return;
	}

	GetStatComponent()->TrySpendStamina(Cost);
}

void APlayerBase::UpdateChargeAttack(float DeltaTime)
{
	if (ChargeAttackPhase == EChargeAttackPhase::None || !GetAttackComponent()) return;
	if (!GetAttackComponent()->IsChargePreparing())
	{
		// 몽타주가 외부 요인으로 종료되었는데 플레이어 측 차지 상태만 남으면
		// 이후 입력과 무관하게 차지 중인 것으로 고착될 수 있다.
		ResetChargeAttackRuntime();
		return;
	}

	ChargeElapsed += FMath::Max(0.0f, DeltaTime);
	const float MaxDuration = FMath::Max(ActiveChargeAttackData.ChargeSettings.MaxChargeDuration, 0.01f);
	if (ChargeTransitionRequest == EChargeTransitionRequest::None && ChargeElapsed >= MaxDuration)
	{
		ChargeTransitionRequest = EChargeTransitionRequest::Attack;
	}

	if (ChargeAttackPhase == EChargeAttackPhase::Begin)
	{
		if (GetAttackComponent()->IsActiveMontageSection(
			ActiveChargeAttackData.ChargeSettings.LoopSectionName))
		{
			ChargeAttackPhase = EChargeAttackPhase::Loop;
		}
		else
		{
			return;
		}
	}

	if (ChargeTransitionRequest == EChargeTransitionRequest::Attack)
	{
		CommitChargeAttack();
	}
	else if (ChargeTransitionRequest == EChargeTransitionRequest::End)
	{
		EndChargeAttack();
	}
}

void APlayerBase::RequestChargeTransition(EChargeTransitionRequest Request)
{
	if (ChargeAttackPhase == EChargeAttackPhase::None || ChargeTransitionRequest != EChargeTransitionRequest::None)
	{
		return;
	}
	ChargeTransitionRequest = Request;
	if (ChargeAttackPhase == EChargeAttackPhase::Loop)
	{
		UpdateChargeAttack(0.0f);
	}
}

void APlayerBase::CommitChargeAttack()
{
	const float MaxDuration = FMath::Max(ActiveChargeAttackData.ChargeSettings.MaxChargeDuration, 0.01f);
	const float ChargeRatio = FMath::Clamp(ChargeElapsed / MaxDuration, 0.0f, 1.0f);
	const FAttackModifiers CommittedModifiers = ActiveChargeAttackData.ResolveModifiers(ChargeRatio);
	const float Cost = GetAttackStaminaCost(ActiveChargeAttackName, &CommittedModifiers);
	if (!GetStatComponent()->CanAffordStamina(Cost) ||
		!GetAttackComponent()->CommitChargeAttack(CommittedModifiers))
	{
		EndChargeAttack();
		return;
	}

	GetStatComponent()->TrySpendStamina(Cost);
	ResetChargeAttackRuntime();
}

void APlayerBase::EndChargeAttack()
{
	if (!GetAttackComponent()->EndChargeAttack())
	{
		GetAttackComponent()->CancelAttack(EActionExitReason::Interrupted, true);
	}
	ResetChargeAttackRuntime();
}

void APlayerBase::ResetChargeAttackRuntime()
{
	ActiveChargeAttackName = NAME_None;
	ActiveChargeAttackData = FBaseAttackData{};
	ChargeAttackPhase = EChargeAttackPhase::None;
	ChargeTransitionRequest = EChargeTransitionRequest::None;
	ChargeElapsed = 0.0f;
}

void APlayerBase::ExecuteJump()
{
	if (CharacterBaseAnim->GetCurrentActiveMontage())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &Super::Jump);
	}
	else
	{
		Super::Jump();
	}
}

void APlayerBase::ExecuteDodge()
{
	if (ActiveDodgeMontage)
	{
		ExitDodgeRuntime(EActionExitReason::Transition);
	}

	const float Cost = GetDodgeStaminaCost();
	if (!GetStatComponent()->TrySpendStamina(Cost))
	{
		FinishActionIfCurrent(TAG_Action_Dodge);
		return;
	}

	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	FVector ValueX = UKismetMathLibrary::GetForwardVector(YawRotation) * InputVector.Y;
	FVector ValueY = UKismetMathLibrary::GetRightVector(YawRotation) * InputVector.X;
	FVector DirectionVector = ValueY + ValueX;
	FVector InputDegree = GetActorTransform().InverseTransformVectorNoScale(DirectionVector);
	float InputY = InputDegree.X;
	float InputX = InputDegree.Y;

	DodgeVector = FVector(InputX, InputY, 0.0f);

	float DodgeDegree;

	if (DodgeVector.X == 0.0f && DodgeVector.Y == 0.0f)
		DodgeDegree = 180.0f;
	else
		DodgeDegree = UKismetMathLibrary::DegAtan2(DodgeVector.X, DodgeVector.Y);

	TArray<float> Angles = { -180.0f, -135.0f, -90.0f, -45.0f, 0.0f, 45.0f, 90.0f, 135.0f, 180.0f };

	float Closest = Angles[0];
	float MinDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(DodgeDegree, Closest));

	for (int32 i = 1; i < Angles.Num(); ++i)
	{
		float Diff = FMath::Abs(FMath::FindDeltaAngleDegrees(DodgeDegree, Angles[i]));
		if (Diff < MinDiff)
		{
			MinDiff = Diff;
			Closest = Angles[i];
		}
	}

	static const TMap<float, FName> AngleToDirection = {
		{ 0.0f, FName("Roll_F")},
		{ 45.0f, FName("Roll_FR")},
		{ 90.0f, FName("Roll_R")},
		{ 135.0f, FName("Roll_BR")},
		{ 180.0f, FName("Roll_B")},
		{ -45.0f, FName("Roll_FL")},
		{ -90.0f, FName("Roll_L")},
		{ -135.0f, FName("Roll_BL")},
		{ -180.0f, FName("Roll_B")},
	};

	FName RollDirectionName = AngleToDirection[Closest];

	UAnimMontage* DodgeMontage = ConfiguredDodgeMontage.Get();
	if (!CharacterBaseAnim || !DodgeMontage)
	{
		FinishActionIfCurrent(TAG_Action_Dodge);
		return;
	}

	if (CharacterBaseAnim->Montage_Play(DodgeMontage) <= 0.0f)
	{
		FinishActionIfCurrent(TAG_Action_Dodge);
		return;
	}
	ActiveDodgeMontage = DodgeMontage;
	CharacterBaseAnim->Montage_JumpToSection(RollDirectionName, DodgeMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &APlayerBase::OnDodgeMontageEnded);
	CharacterBaseAnim->Montage_SetEndDelegate(EndDelegate, DodgeMontage);
}

void APlayerBase::TryReturnToLocomotion(const FVector2D& MovementInput)
{
	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	if (!StatusComponent || MovementInput.IsNearlyZero() ||
		!StatusComponent->IsWindowOpen(TAG_Window_Locomotion))
	{
		return;
	}

	const FGameplayTag CurrentAction = StatusComponent->GetCurrentAction();
	if (!CurrentAction.MatchesTagExact(TAG_Action_Attack) &&
		!CurrentAction.MatchesTagExact(TAG_Action_Dodge) &&
		!CurrentAction.MatchesTagExact(TAG_Action_Guard) &&
		!CurrentAction.MatchesTagExact(TAG_Action_Parry) &&
		!CurrentAction.MatchesTagExact(TAG_Action_HitReact))
	{
		return;
	}

	ExitActionRuntime(CurrentAction, EActionExitReason::Locomotion);
	FinishActionIfCurrent(CurrentAction);
}

void APlayerBase::RefreshActionAnimationProfile(EWeaponType WeaponType)
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UPlayerAnimRegistrySubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UPlayerAnimRegistrySubsystem>() : nullptr;
	if (!Registry)
	{
		ConfiguredDodgeMontage = nullptr;
		ConfiguredParryMontage = nullptr;
		ConfiguredCriticalExecutions.Reset();
		DodgeLocomotionBlendOutTime = 0.15f;
		DodgeExitBlendSettings = FActionExitBlendSettings{};
		ParryExitBlendSettings = FActionExitBlendSettings{};
		return;
	}

	const FPlayerAnimSet AnimSet = Registry->ResolvePlayerAnimSet(WeaponType);
	ConfiguredDodgeMontage = AnimSet.DodgeMontage.LoadSynchronous();
	ConfiguredParryMontage = AnimSet.ParryMontage.LoadSynchronous();
	ConfiguredCriticalExecutions = AnimSet.CriticalExecutions;
	DodgeLocomotionBlendOutTime = AnimSet.DodgeLocomotionBlendOutTime >= 0.0f
		? AnimSet.DodgeLocomotionBlendOutTime : 0.15f;
	DodgeExitBlendSettings = AnimSet.DodgeExitBlendSettings;
	ParryExitBlendSettings = AnimSet.ParryExitBlendSettings;
	if (DodgeExitBlendSettings.Locomotion < 0.0f && AnimSet.DodgeLocomotionBlendOutTime >= 0.0f)
	{
		DodgeExitBlendSettings.Locomotion = AnimSet.DodgeLocomotionBlendOutTime;
	}
}

void APlayerBase::ExecuteBlock()
{
	IsBlockInput = true;
}

void APlayerBase::ExecuteParry()
{
	if (GetHitReactionComponent())
	{
		GetHitReactionComponent()->ResetParryActiveWindow();
	}

	UAnimMontage* ParryMontage = ConfiguredParryMontage.Get();
	if (!CharacterBaseAnim || !ParryMontage || CharacterBaseAnim->Montage_Play(ParryMontage) <= 0.0f)
	{
		FinishActionIfCurrent(TAG_Action_Parry);
		return;
	}

	ActiveParryMontage = ParryMontage;
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &APlayerBase::OnParryMontageEnded);
	CharacterBaseAnim->Montage_SetEndDelegate(EndDelegate, ParryMontage);
}

void APlayerBase::ExecuteInteract()
{
	if (IsInteraction)
	{
		if (GetCharacterStatusComponent()->GetCurrentState().MatchesTagExact(TAG_State_Ladder))
		{
			FinishActionIfCurrent(TAG_Action_Interact);
			return;
		}

		InteractComponent->CancelMoveToInteractPos();
		GetController()->SetIgnoreMoveInput(false);
		GetController()->StopMovement();
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		IsInteraction = false;
		GetCharacterStatusComponent()->ClearAction();
		return;
	}
	else
	{
		bool InteractTargetValid = InteractComponent->SetInteractActorByDegree(this, 60.0f);

		if (!InteractTargetValid)
		{
			FinishActionIfCurrent(TAG_Action_Interact);
			return;
		}

		GetController()->SetIgnoreMoveInput(true);
		IsInteraction = InteractComponent->MovetoInteractPos();
		if (!IsInteraction)
		{
			GetController()->SetIgnoreMoveInput(false);
			GetCharacterStatusComponent()->ClearAction();
		}
	}
}

void APlayerBase::HandleAttackFinished(bool bInterrupted)
{
	IsAttackInput = false;
	ResetChargeAttackRuntime();
	bForcedRotatingInputDirection = false;
	FinishActionIfCurrent(TAG_Action_Attack);
}

void APlayerBase::FinishActionIfCurrent(const FGameplayTag& ExpectedAction)
{
	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	if (StatusComponent && StatusComponent->GetCurrentAction().MatchesTagExact(ExpectedAction))
	{
		StatusComponent->ClearAction();
	}
}

void APlayerBase::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveDodgeMontage)
	{
		return;
	}
	ActiveDodgeMontage = nullptr;
	FinishActionIfCurrent(TAG_Action_Dodge);
}

void APlayerBase::OnParryMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveParryMontage)
	{
		return;
	}

	ActiveParryMontage = nullptr;
	if (GetHitReactionComponent())
	{
		GetHitReactionComponent()->ResetParryActiveWindow();
	}
	FinishActionIfCurrent(TAG_Action_Parry);
}

void APlayerBase::OnCriticalExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveCriticalExecutionMontage)
	{
		return;
	}

	AEnemyBase* ExecutionTarget = ActiveCriticalExecutionTarget.Get();
	ActiveCriticalExecutionTarget = nullptr;
	ActiveCriticalExecutionMontage = nullptr;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (ExecutionTarget)
	{
		ExecutionTarget->FinishCriticalExecution(this, !bInterrupted);
	}
	FinishActionIfCurrent(TAG_Action_Execution);
}

void APlayerBase::HandleActionTransition(const FGameplayTag& PreviousAction,
	const FGameplayTag& NextAction, EActionExitReason ExitReason)
{
	if (!PreviousAction.IsValid() || PreviousAction.MatchesTagExact(NextAction))
	{
		return;
	}

	ExitActionRuntime(PreviousAction, ExitReason);
}

void APlayerBase::ExitActionRuntime(const FGameplayTag& ActionTag, EActionExitReason ExitReason)
{
	if (ActionTag.MatchesTagExact(TAG_Action_Attack))
	{
		IsAttackInput = false;
		PendingAttackName = NAME_None;
		ResetChargeAttackRuntime();
		bForcedRotatingInputDirection = false;
		if (GetAttackComponent())
		{
			GetAttackComponent()->CancelAttack(ExitReason, true);
		}
	}
	else if (ActionTag.MatchesTagExact(TAG_Action_Dodge))
	{
		ExitDodgeRuntime(ExitReason);
	}
	else if (ActionTag.MatchesTagExact(TAG_Action_Parry))
	{
		ExitParryRuntime(ExitReason);
	}
	else if (ActionTag.MatchesTagExact(TAG_Action_Execution))
	{
		ExitCriticalExecutionRuntime(ExitReason);
	}
	else if (ActionTag.MatchesTagExact(TAG_Action_Guard))
	{
		IsBlockInput = false;
	}
	else if (ActionTag.MatchesTagExact(TAG_Action_HitReact))
	{
		if (GetHitReactionComponent())
		{
			GetHitReactionComponent()->CancelHitReaction(ExitReason, true);
		}
	}
}

void APlayerBase::ExitDodgeRuntime(EActionExitReason ExitReason)
{
	UAnimMontage* DodgeMontage = ActiveDodgeMontage.Get();
	ActiveDodgeMontage = nullptr;
	if (!CharacterBaseAnim || !DodgeMontage)
	{
		return;
	}

	FOnMontageEnded EmptyDelegate;
	CharacterBaseAnim->Montage_SetEndDelegate(EmptyDelegate, DodgeMontage);
	if (CharacterBaseAnim->Montage_IsPlaying(DodgeMontage))
	{
		FAlphaBlendArgs BlendOut = DodgeMontage->GetBlendOutArgs();
		const float OverrideTime = DodgeExitBlendSettings.GetOverride(ExitReason);
		if (OverrideTime >= 0.0f)
		{
			BlendOut.BlendTime = OverrideTime;
		}
		CharacterBaseAnim->Montage_StopWithBlendOut(BlendOut, DodgeMontage);
	}
}

void APlayerBase::HandleLadderExit()
{
	IsInteraction = false;
	ClimbComponent->ClearRepeatedClimbInput();

	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	if (!StatusComponent->IsDead())
	{
		StatusComponent->SetState(TAG_State_Ground);
	}
}

/* ============================================================
 *  Landed
 * ============================================================ */
void APlayerBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (RideComponent)
	{
		RideComponent->HandlePlayerLanded();
	}
	SetSkipJumpStart(false);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	FinishActionIfCurrent(TAG_Action_Jump);
}

/* ============================================================
 *  Locomotion
 * ============================================================ */
void APlayerBase::Walk()
{
	CurLocomotionGait = ELocomotionGait::Walk;
	GetCharacterMovement()->MaxWalkSpeed = 200.0f;
}

void APlayerBase::Jog()
{
	CurLocomotionGait = ELocomotionGait::Jog;
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	GetCharacterMovement()->BrakingFriction = 0.3f;
}

void APlayerBase::Sprint()
{
	if (GetStatComponent()->GetStamina() <= 0.f) { Jog(); return; }

	CurLocomotionGait = ELocomotionGait::Sprint;
	if (!LockOnComponent->IsLockedOn())
	{
		GetCharacterMovement()->MaxWalkSpeed = 600.0f;
		GetCharacterMovement()->BrakingFriction = 0.1f;
	}
}

float APlayerBase::GetDirection()
{
	return Direction;
}

void APlayerBase::SetRotationInputDirection_Implementation()
{
	// 락온 중 공격 방향은 이동 입력이 아니라 현재 전투 대상을 우선한다.
	// 횡이동 입력으로 인해 공격 보정과 락온 회전이 서로 경쟁하는 것을 방지한다.
	if (AActor* LockOnTarget = LockOnComponent ? LockOnComponent->GetCurrentTarget() : nullptr)
	{
		const FVector ToTarget = LockOnTarget->GetActorLocation() - GetActorLocation();
		if (!ToTarget.IsNearlyZero())
		{
			InputRotation = ToTarget.Rotation();
			InputRotation.Pitch = 0.0f;
			InputRotation.Roll = 0.0f;
			bForcedRotatingInputDirection = true;
			return;
		}
	}

	FVector LastMovementInput = GetLastMovementInputVector();
	if (!LastMovementInput.IsNearlyZero())
	{
		InputRotation = LastMovementInput.Rotation();
		bForcedRotatingInputDirection = true;
	}
}

bool APlayerBase::GetIsMovementInput()
{
	return IsMovementInput;
}

FVector APlayerBase::GetInputDirection()
{
	return DodgeVector;
}

UStaticMeshComponent* APlayerBase::GetMainWeaponMesh() const
{
	return EquipmentComponent ? EquipmentComponent->GetMainWeaponComponent() : nullptr;
}

/* ============================================================
 *  Interface Implementations
 * ============================================================ */
bool APlayerBase::IsPlayer_Implementation()
{
	IPlayerInterface::IsPlayer_Implementation();
	return false;
}

void APlayerBase::RegisterInteractableActor_Implementation(AActor* Interactable)
{
	IPlayerInterface::RegisterInteractableActor_Implementation(Interactable);
	InteractComponent->AddInteractObject(Interactable);
}

void APlayerBase::DeRegisterInteractableActor_Implementation(AActor* Interactable)
{
	IPlayerInterface::DeRegisterInteractableActor_Implementation(Interactable);
	InteractComponent->RemoveInteractObject(Interactable);
}

void APlayerBase::EndInteraction_Implementation(AActor* Interactable)
{
	IPlayerInterface::EndInteraction_Implementation(Interactable);
}

void APlayerBase::HandleArrivedInteractionPoint()
{
	AActor* InteractActor = InteractComponent->GetInteractActor();
	if (!IsValid(InteractActor))
	{
		HandleInteractionMoveCancelled();
		return;
	}

	GetController()->SetIgnoreMoveInput(false);

	GetCharacterStatusComponent()->ClearAction();

	if (InteractActor->ActorHasTag("Ladder"))
	{
		const bool bRequestSucceeded = ClimbComponent->RequestEnterLadder(InteractActor);
		if (!bRequestSucceeded)
		{
			IsInteraction = false;
			return;
		}

		GetCharacterStatusComponent()->SetState(TAG_State_Ladder);
	}
	else if (InteractActor->ActorHasTag("NPC"))
	{
		IInteractInterface::Execute_Interact(InteractActor, this);
	}

	IsInteraction = true;
}

void APlayerBase::HandleInteractionMoveCancelled()
{
	IsInteraction = false;
	if (AController* OwningController = GetController())
	{
		OwningController->SetIgnoreMoveInput(false);
		OwningController->StopMovement();
	}

	if (!GetCharacterStatusComponent()->IsDead())
	{
		GetCharacterStatusComponent()->ClearAction();
	}
}

/* ============================================================
 *  Hit Reaction
 * ============================================================ */
void APlayerBase::OnHit_Implementation(const FAttackRequest& AttackInfo)
{
	if (IsCriticalExecutionActive())
	{
		return;
	}

	const FHitResolution Resolution = GetHitReactionComponent()->ResolveHit(AttackInfo);
	const float HitAngle = Resolution.HitAngle;

	if (Resolution.Outcome == EHitOutcome::Avoided)
	{
		return;
	}

	if (Resolution.Outcome == EHitOutcome::Parried)
	{
		if (IAttackSourceInterface* AttackSource = Cast<IAttackSourceInterface>(AttackInfo.AttackCauser))
		{
			AttackSource->ReceiveParried(this);
		}
		return;
	}

	ECombatReaction Response = Resolution.Reaction;

	if (Resolution.Outcome == EHitOutcome::Hit)
	{
		bool bPoiseBroken = false;
		if (!ApplyDirectHitStats(AttackInfo, bPoiseBroken))
		{
			return;
		}

		switch (Response)
		{
		case ECombatReaction::None:
			if (bPoiseBroken)
			{
				RestorePoise();
			}
			return;

		case ECombatReaction::Flinch:
		case ECombatReaction::KnockBack:
		case ECombatReaction::KnockDown:
		{
			const bool bUpgradeActiveReaction =
				GetHitReactionComponent()->CanUpgradeActiveReaction(Response);
			if (bPoiseBroken || bUpgradeActiveReaction)
			{
				const FHitReactionRequest InputReaction = { Response, HitAngle };
				const bool bExecuted = bUpgradeActiveReaction
					? GetHitReactionComponent()->ExecuteHitResponse(InputReaction)
					: TryExecuteHitReaction(InputReaction);
				if (!bExecuted)
				{
					RestorePoise();
				}
			}
			return;
		}

		case ECombatReaction::HitAir:
			if (bPoiseBroken && GetCharacterStatusComponent()->CanTryAction(TAG_Action_HitReact))
			{
				const FHitReactionRequest InputReaction = { ECombatReaction::HitAir, HitAngle };
				if (!TryExecuteHitReaction(InputReaction))
				{
					RestorePoise();
				}
			}
			return;

		default:
			UE_LOG(Log_Hit, Warning, TEXT("[PlayerBase] Unexpected direct-hit response: %s"),
				*StaticEnum<ECombatReaction>()->GetNameStringByValue(static_cast<int64>(Response)));
			return;
		}
	}

	switch (Response)
	{
	case ECombatReaction::GuardHit:
	case ECombatReaction::GuardHitHeavy:
	{
		float PerformanceRatio = GetStatComponent()->GetWeaponPerformanceRatio(EquipmentComponent->GetEquipedWeapon()->RequiredAttributes.ToCharacterStats());
		float GuardBoost = EquipmentComponent->GetEquipedWeapon()->GuardBoost;
		float GuardNegation = EquipmentComponent->GetEquipedWeapon()->GuardNegation;
		if (PerformanceRatio < 1.0f)
		{
			GuardBoost *= 0.8f;
			GuardNegation *= 0.8f;
		}

		float ApplyGuardBoost = AttackInfo.StanceDamage * (1.0f - GuardBoost / 100.0f);
		bool IsStaminaEnough = GetStatComponent()->ChangeStamina(ApplyGuardBoost, EStatChangeType::Damage);
		if (IsStaminaEnough)
		{
			float ApplyNegationDamage = AttackInfo.Damage * (1.0f - GuardNegation / 100.0f);
			StatComponent->ApplyDamage(ApplyNegationDamage, AttackInfo.AttackType);
			if (!GetCharacterStatusComponent()->IsDead())
			{
				FHitReactionRequest InputReaction = { Response, HitAngle };
				TryExecuteHitReaction(InputReaction);
			}
		}
		else
		{
			StatComponent->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
			Response = ECombatReaction::GuardBreak;
			GuardReentryLockoutRemaining = GuardReentryLockoutDuration;
			if (UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent())
			{
				StatusComponent->RemoveBufferedAction(TAG_Action_Guard);
			}
			FHitReactionRequest InputReaction = { Response, HitAngle };
			TryExecuteHitReaction(InputReaction);
		}
		break;
	}
	}
}

bool APlayerBase::TryExecuteHitReaction(const FHitReactionRequest& ReactionRequest)
{
	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	UPlayerHitReactionComponent* ReactionComponent = GetHitReactionComponent();
	if (!StatusComponent || !ReactionComponent || StatusComponent->IsDead() ||
		!StatusComponent->CanTryAction(TAG_Action_HitReact))
	{
		return false;
	}

	StatusComponent->SwitchAction(TAG_Action_HitReact, EActionExitReason::Interrupted);
	if (!ReactionComponent->ExecuteHitResponse(ReactionRequest))
	{
		FinishActionIfCurrent(TAG_Action_HitReact);
		return false;
	}

	return true;
}

void APlayerBase::ExitCriticalExecutionRuntime(EActionExitReason ExitReason)
{
	AEnemyBase* ExecutionTarget = ActiveCriticalExecutionTarget.Get();
	UAnimMontage* ExecutionMontage = ActiveCriticalExecutionMontage.Get();
	ActiveCriticalExecutionTarget = nullptr;
	ActiveCriticalExecutionMontage = nullptr;

	if (CharacterBaseAnim && ExecutionMontage)
	{
		FOnMontageEnded EmptyDelegate;
		CharacterBaseAnim->Montage_SetEndDelegate(EmptyDelegate, ExecutionMontage);
		if (CharacterBaseAnim->Montage_IsPlaying(ExecutionMontage))
		{
			CharacterBaseAnim->Montage_Stop(0.1f, ExecutionMontage);
		}
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (ExecutionTarget)
	{
		ExecutionTarget->FinishCriticalExecution(this, false);
	}
}

void APlayerBase::ExitParryRuntime(EActionExitReason ExitReason)
{
	UAnimMontage* ParryMontage = ActiveParryMontage.Get();
	ActiveParryMontage = nullptr;
	if (GetHitReactionComponent())
	{
		GetHitReactionComponent()->ResetParryActiveWindow();
	}
	if (!CharacterBaseAnim || !ParryMontage)
	{
		return;
	}

	FOnMontageEnded EmptyDelegate;
	CharacterBaseAnim->Montage_SetEndDelegate(EmptyDelegate, ParryMontage);
	if (CharacterBaseAnim->Montage_IsPlaying(ParryMontage))
	{
		FAlphaBlendArgs BlendOut = ParryMontage->GetBlendOutArgs();
		const float OverrideTime = ParryExitBlendSettings.GetOverride(ExitReason);
		if (OverrideTime >= 0.0f)
		{
			BlendOut.BlendTime = OverrideTime;
		}
		CharacterBaseAnim->Montage_StopWithBlendOut(BlendOut, ParryMontage);
	}
}

void APlayerBase::HandleDeathStarted()
{
	Super::HandleDeathStarted();
	if (LockOnComponent)
	{
		LockOnComponent->ClearLockOn();
	}

	if (GetAttackComponent())
	{
		GetAttackComponent()->CancelAttack(EActionExitReason::Death, true);
	}
	bForcedRotatingInputDirection = false;
	IsAttackInput = false;
	bAttackButtonHeld = false;
	bDodgeSprintInputHeld = false;
	bSprintStartedByHold = false;
	DodgeSprintHeldTime = 0.0f;
	PendingAttackName = NAME_None;
	ResetChargeAttackRuntime();
	IsBlockInput = false;
	bWantsToGuard = false;

	// 입력 차단
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	InteractComponent->CancelMoveToInteractPos();
	IsInteraction = false;

	// 이전 State별 뒷정리
	const FGameplayTag PrevState = GetCharacterStatusComponent()->GetPreviousStateBeforeDeath();

	if (PrevState.MatchesTagExact(TAG_State_Ride))
	{
		// 사망 중에는 일반 하차 애니메이션을 거치지 않고 세션 상태를 강제로 정리한다.
		if (RideComponent)
		{
			RideComponent->ForceDetachFromRide(true);
		}
	}
	// Ladder cleanup is owned by ClimbComponent's OnDeathStarted subscription.
	// State.Ground / 그 외: 별도 정리 없음
}

void APlayerBase::HandleDeathFinalized()
{
	Super::HandleDeathFinalized(); // 캡슐 콜리전 off
}

void APlayerBase::HandleRespawnStarted()
{
	// 1. 메시 복구 (래그돌 안 쓰는 정책이면 사실 거의 안 필요하지만 안전 차원)
	GetMesh()->SetSimulatePhysics(false);

	// 2. 체력/스태미나/포커스 풀로 복원
	if (UPlayerStatComponent* PlayerStat = GetStatComponent())
	{
		PlayerStat->InitializeStats();
	}

	// 부활 몽타주 재생은 AnimInstance가 OnRespawnStarted 듣고 알아서 처리
	// 몽타주 끝 노티파이가 FinalizeRespawn 호출
}

void APlayerBase::HandleRespawnFinalized()
{
	Super::HandleRespawnFinalized();  // 캡슐 콜리전 복구

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}
}

/* ============================================================
 *  Ride
 * ============================================================ */


/* ============================================================
 *  View Data Interface
 * ============================================================ */
FTransform APlayerBase::GetCameraTransform() const
{
	return Camera->GetComponentTransform();
}

void APlayerBase::RefreshPlayerCameraComponents()
{
	SpringArm->UpdateComponentToWorld();
	Camera->UpdateComponentToWorld();
}

FTransform APlayerBase::GetSpringArmTransform() const
{
	return SpringArm->GetComponentTransform();
}

float APlayerBase::GetTargetArmLength() const
{
	return SpringArm->TargetArmLength;
}

FRotator APlayerBase::GetControllerRotation() const
{
	return GetController()->GetControlRotation();
}

TOptional<FVector> APlayerBase::GetCharBoneLocation(FName BoneName)
{
	return GetMesh()->DoesSocketExist(BoneName) ? TOptional<FVector>(GetMesh()->GetSocketLocation(BoneName)) : TOptional<FVector>();
}


/* ============================================================
 *  Component Getters
 * ============================================================ */
UPlayerAttackComponent* APlayerBase::GetAttackComponent() const
{
	return Cast<UPlayerAttackComponent>(AttackComponent);
}

UPlayerHitReactionComponent* APlayerBase::GetHitReactionComponent() const
{
	return Cast<UPlayerHitReactionComponent>(HitReactionComponent);
}

UPlayerStatusComponent* APlayerBase::GetCharacterStatusComponent() const
{
	return Cast<UPlayerStatusComponent>(CharacterStatusComponent);
}

UPlayerStatComponent* APlayerBase::GetStatComponent() const
{
	return Cast<UPlayerStatComponent>(StatComponent);
}

/* ============================================================
 *  Attack Source Interface
 * ============================================================ */
FAttackTraceSource APlayerBase::GetAttackTraceSource(EAttackSourceType AttackSourceType) const
{
	if (!EquipmentComponent) return FAttackTraceSource();
	return EquipmentComponent->GetAttackTraceSource(AttackSourceType);
}

FAttackDamageSource APlayerBase::GetAttackDamageSource() const
{
	if (!EquipmentComponent) return FAttackDamageSource();
	return EquipmentComponent->GetAttackDamageSource();
}

void APlayerBase::ReceiveParried(AActor* ParryInstigator)
{
	UPlayerStatusComponent* StatusComponent = GetCharacterStatusComponent();
	UPlayerHitReactionComponent* ReactionComponent = GetHitReactionComponent();
	if (!StatusComponent || !ReactionComponent || StatusComponent->IsDead())
	{
		return;
	}

	if (GetAttackComponent())
	{
		GetAttackComponent()->CancelAttack(EActionExitReason::Interrupted, true);
	}

	const FVector InstigatorLocation = IsValid(ParryInstigator)
		? ParryInstigator->GetActorLocation() : GetActorLocation() + GetActorForwardVector();
	const FHitReactionRequest ReactionRequest{
		ECombatReaction::StanceBreak, ReactionComponent->CalculateHitAngle(InstigatorLocation) };

	StatusComponent->SwitchAction(TAG_Action_HitReact, EActionExitReason::Interrupted);
	if (!ReactionComponent->ExecuteHitResponse(ReactionRequest))
	{
		FinishActionIfCurrent(TAG_Action_HitReact);
	}
}

/* ============================================================
 *  LockOn
 * ============================================================ */
void APlayerBase::SetLockOnMovementMode(bool bLockOn)
{
	// 카메라의 수동 오프셋과 캐릭터 몸 회전을 분리한다. 락온 중 몸 회전은
	// LockOnComponent가 타깃 방향으로 직접 갱신한다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = !bLockOn;
	Jog();
}

void APlayerBase::HandleLockOnTargetChanged(AActor* NewTarget)
{
	SetLockOnMovementMode(IsValid(NewTarget));
}

void APlayerBase::OnLockOnToggle()
{
	UE_LOG(Log_LockOn, Warning, TEXT("[APlayerBase] %s Execute LockOn Toggle"), *GetName());
	if (LockOnComponent)
	{
		UE_LOG(Log_LockOn, Warning, TEXT("[APlayerBase] %s Access LockOn Allowed"), *GetName());
		LockOnComponent->ToggleLockOn();
	}
}

void APlayerBase::OnLockOnSwitchLeft()
{
	if (LockOnComponent) LockOnComponent->SwitchTarget(false);
}

void APlayerBase::OnLockOnSwitchRight()
{
	if (LockOnComponent) LockOnComponent->SwitchTarget(true);
}
