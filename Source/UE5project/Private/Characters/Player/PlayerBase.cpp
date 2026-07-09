// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Player/PlayerBase.h"

// 이동
#include "Characters/Components/BaseCharacterMovementComponent.h"

// 콜리전
#include "Components/CapsuleComponent.h"

// 카메라
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

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
#include "Characters/Rideable/Interfaces/RideInterface.h"

// 유저 컴포넌트
#include "Characters/Player/Components/PlayerStatusComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Player/Components/InventoryComponent.h" 
#include "Combat/Components/CombatComponent.h"
#include "Combat/Components/PlayerAttackComponent.h"
#include "Combat/Components/PlayerHitReactionComponent.h"
#include "Interaction/Components/InteractComponent.h"
#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Characters/Player/Components/LockOnComponent.h"

// 데이터 참조
#include "Characters/Player/PlayerConfig.h"

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

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Character_Player"));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	GetCapsuleComponent()->SetCapsuleHalfHeight(90.0f);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> RollMontageAsset(TEXT("/Game/04_Animations/Player/SSH/Roll/Normal/Roll_Montage.Roll_Montage"));
	if (RollMontageAsset.Succeeded())
	{
		RollMontage = RollMontageAsset.Object;
	}

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

	CharacterBaseAnim = Cast<UPlayerBaseAnimInstance>(GetMesh()->GetAnimInstance());

	if (CharacterBaseAnim)
	{
		CharacterBaseAnim->OnMountEnd.AddUObject(this, &APlayerBase::MountEnd);
		CharacterBaseAnim->OnDisMountEnd.AddUObject(this, &APlayerBase::DisMountEnd);
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			SubSystem->AddMappingContext(InputConfig->DefaultContext, 0);
		}
	}

	InitSpringArmLocation = SpringArm->GetRelativeLocation();
}

/* ============================================================
 *  Tick
 * ============================================================ */
void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

		if (!LockOnComponent->IsLockedOn())
		{
			SetLockOnMovementMode(false);
			return;
		}

		ApplyLockOnRotation(DeltaTime);
	}

	if (GetStatComponent())
	{
		if (CurLocomotionGait == ELocomotionGait::Sprint && GetVelocity().SizeSquared() > 100.f)
		{
			GetStatComponent()->ChangeStamina(SprintStaminaPerSec * DeltaTime, EStatChangeType::Damage);
			if (GetStatComponent()->GetStamina() <= 0.f)
				Jog();   // 바닥나면 조그로 강제 전환
		}

		GetStatComponent()->TickStaminaRegen(DeltaTime);
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

	GetCharacterStatusComponent()->WindowRules = Config->WindowRules;
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
		EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Triggered, this, &APlayerBase::DodgeInput);

		EnhancedInputComponent->BindAction(InputConfig->Block, ETriggerEvent::Ongoing, this, &APlayerBase::BlockInput);
		EnhancedInputComponent->BindAction(InputConfig->Block, ETriggerEvent::Triggered, this, &APlayerBase::BlockInputEnd);

		//EnhancedInputComponent->BindAction(InputConfig->Parry, ETriggerEvent::Triggered, this, &APlayerBase::ParryInput);

		EnhancedInputComponent->BindAction(InputConfig->Interact, ETriggerEvent::Triggered, this, &APlayerBase::InteractInput);

		EnhancedInputComponent->BindAction(InputConfig->Sprint, ETriggerEvent::Started, this, &APlayerBase::Sprint);
		EnhancedInputComponent->BindAction(InputConfig->Sprint, ETriggerEvent::Triggered, this, &APlayerBase::Jog);

		EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Started, this, &APlayerBase::Walk);
		EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Triggered, this, &APlayerBase::Jog);

		EnhancedInputComponent->BindAction(InputConfig->SpawnRide, ETriggerEvent::Triggered, this, &APlayerBase::SpawnRideInput);

		EnhancedInputComponent->BindAction(InputConfig->Attack, ETriggerEvent::Started, this, &APlayerBase::AttackInput);
		EnhancedInputComponent->BindAction(InputConfig->Attack, ETriggerEvent::Triggered, this, &APlayerBase::AttackInputEnd);

		EnhancedInputComponent->BindAction(InputConfig->Modifier, ETriggerEvent::Ongoing, this, &APlayerBase::ModifierInput);
		EnhancedInputComponent->BindAction(InputConfig->Modifier, ETriggerEvent::Triggered, this, &APlayerBase::ModifierInputEnd);

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

	if (GetCharacterStatusComponent())
	{
		// ★ 버퍼에서 소비된 행동의 실제 실행을 위한 바인딩
		GetCharacterStatusComponent()->OnActionConsumed.BindUObject(this, &APlayerBase::HandleBufferedAction);

		if(GetAttackComponent()) GetAttackComponent()->OnAttackFinished.AddUObject(this, &APlayerBase::OnActionFinished);
		if(GetClimbComponent()) GetClimbComponent()->OnLadderExit.AddUObject(this, &APlayerBase::OnStateChanged, TAG_State_Ground.GetTag());
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
		ExecuteBlock();
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
}

/* ============================================================
 *  Combat Input — 판단부 (RequestAction으로 체크/버퍼)
 * ============================================================ */
void APlayerBase::AttackInput()
{
	if (GetStatComponent()->GetStamina() <= 0.f) return;

	if (GetCharacterStatusComponent()->RequestAction(TAG_Action_Attack))
	{
		UE_LOG(Log_Character_Player_Input, Error, TEXT("[APlayerBase] Can Attack Action"));
		ExecuteAttack(); // 즉시 가능하면 바로 실행
	}
	// else: 버퍼에 저장됨 → Window 열리면 HandleBufferedAction → ExecuteAttack
}

void APlayerBase::AttackInputEnd()
{
	IsAttackInput = false;
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
	if (GetStatComponent()->GetStamina() <= 0.f) return;

	if (GetCharacterStatusComponent()->RequestAction(TAG_Action_Dodge))
	{
		ExecuteDodge();
	}
}

void APlayerBase::BlockInput()
{
	if (!GetCharacterStatusComponent()->CanTryAction(TAG_Action_Guard)) return;
	ExecuteBlock();
}

void APlayerBase::BlockInputEnd()
{
	IsBlockInput = false;
	OnActionFinished(false);
}

void APlayerBase::InteractInput()
{
	if (!GetCharacterStatusComponent()->CanTryAction(TAG_Action_Interact)) return;
	ExecuteInteract();
}

void APlayerBase::SpawnRideInput()
{
	ExecuteSpawnRide();
}

/* ============================================================
 *  Combat Execute — 실행부 (순수 로직, 판단 없음)
 * ============================================================ */
void APlayerBase::ExecuteAttack()
{
	GetCharacterStatusComponent()->SwitchAction(TAG_Action_Attack);

	IsAttackInput = true;
	const FBaseAttackData* Played = GetAttackComponent()->ExecuteAttack(FName("DefaultCombo"));
	if (Played)
	{
		if (const FWeaponSetsInfo* Weapon = GetEquipmentComponent()->GetEquipedWeapon())
		{
			const float Cost = Weapon->StaminaCost * Played->StaminaCostMultiplier;
			GetStatComponent()->ChangeStamina(Cost, EStatChangeType::Damage);
		}
	}
}

void APlayerBase::ExecuteJump()
{
	GetCharacterStatusComponent()->SwitchAction(TAG_Action_Jump);

	if (CharacterBaseAnim->GetCurrentActiveMontage())
	{
		CharacterBaseAnim->Montage_Stop(0.1f);
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &Super::Jump);
	}
	else
	{
		Super::Jump();
	}
}

void APlayerBase::ExecuteDodge()
{
	GetCharacterStatusComponent()->SwitchAction(TAG_Action_Dodge);

	const FPlayerStats Stats = GetStatComponent()->GetCharacterStats();
	const float LoadRatio = Stats.EquipLoad.Max > KINDA_SMALL_NUMBER
		? FMath::Clamp(Stats.EquipLoad.Current / Stats.EquipLoad.Max, 0.f, 1.f) : 0.f;
	const float Cost = DodgeStaminaBase * (1.0f + LoadRatio);  // 무부하 ×1 ~ 만적재 ×2
	GetStatComponent()->ChangeStamina(Cost, EStatChangeType::Damage);

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

	CharacterBaseAnim->Montage_Stop(0.1f);
	CharacterBaseAnim->Montage_Play(RollMontage);
	CharacterBaseAnim->Montage_JumpToSection(RollDirectionName, RollMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &APlayerBase::OnDodgeMontageEnded);
	CharacterBaseAnim->Montage_SetEndDelegate(EndDelegate, RollMontage);
}

void APlayerBase::ExecuteBlock()
{
	GetCharacterStatusComponent()->SwitchAction(TAG_Action_Guard);

	IsBlockInput = true;
}

void APlayerBase::ExecuteInteract()
{
	if (IsInteraction)
	{
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
			return;

		GetCharacterStatusComponent()->SwitchAction(TAG_Action_Interact);
		GetController()->SetIgnoreMoveInput(true);
		IsInteraction = InteractComponent->MovetoInteractPos();
	}
}

void APlayerBase::ExecuteSpawnRide()
{
	Ride = GetWorld()->SpawnActor<APlayerRide>(RideClass, GetActorTransform());
	if (!Ride)
	{
		UE_LOG(Log_RideSpawn, Warning, TEXT("[APlayerBase] %s : Horse was Not Spawned"), *GetName());
		return;
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	IRideInterface::Execute_Mount(Ride, this, GetVelocity());
	GetCharacterMovement()->DisableMovement();

	CurRideStance = ERideStance::Mount;
	GetCharacterStatusComponent()->SetState(TAG_State_Ride);

	GetWorldTimerManager().SetTimer(MountTimerHandle, this, &APlayerBase::MountTimer, 0.01f, true);
}

void APlayerBase::OnActionFinished(bool bInterrupted)
{
	if(!bInterrupted) GetCharacterStatusComponent()->ClearAction();
}

void APlayerBase::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	OnActionFinished(bInterrupted);
}

void APlayerBase::OnStateChanged(const FGameplayTag NewState)
{
	GetCharacterStatusComponent()->SetState(NewState);
}

/* ============================================================
 *  Landed
 * ============================================================ */
void APlayerBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	OnActionFinished(false);
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

float APlayerBase::GetRideSpeed()
{
	if (Ride == nullptr) return 0.0f;
	return IRideInterface::Execute_GetRideSpeed(Ride);
}

float APlayerBase::GetRideDirection()
{
	if (Ride == nullptr) return 0.0f;
	return IRideInterface::Execute_GetRideDirection(Ride);
}

FVector APlayerBase::GetInputDirection()
{
	return DodgeVector;
}

UStaticMeshComponent* APlayerBase::GetMainWeaponMesh() const
{
	return EquipmentComponent ? EquipmentComponent->GetMainWeaponComponent() : nullptr;
}

ERideStance APlayerBase::GetCurRideStance()
{
	return CurRideStance;
}

TOptional<FVector> APlayerBase::GetRideIKTargetLoc(EBodyType BoneType)
{
	if (Ride == nullptr)
		return TOptional<FVector>();

	switch (BoneType)
	{
	case EBodyType::Hand_L:
		return Ride->GetMesh()->DoesSocketExist(FName("Reins_Bn_Hand_L"))
			? Ride->GetMesh()->GetSocketLocation(FName("Reins_Bn_Hand_L"))
			: TOptional<FVector>();
	case EBodyType::Hand_R:
		return Ride->GetMesh()->DoesSocketExist(FName("Reins_Bn_Hand_R"))
			? Ride->GetMesh()->GetSocketLocation(FName("Reins_Bn_Hand_R"))
			: TOptional<FVector>();
	case EBodyType::Foot_L:
		return Ride->GetMesh()->DoesSocketExist(FName("SaddleLeftFootPlace"))
			? Ride->GetMesh()->GetSocketLocation(FName("SaddleLeftFootPlace"))
			: TOptional<FVector>();
	case EBodyType::Foot_R:
		return Ride->GetMesh()->DoesSocketExist(FName("SaddleRightFootPlace"))
			? Ride->GetMesh()->GetSocketLocation(FName("SaddleRightFootPlace"))
			: TOptional<FVector>();
	default:
		return TOptional<FVector>();
	}
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
	USceneComponent* InteractionPoint = IInteractInterface::Execute_GetEnterInteractLocation(InteractActor, this);

	GetController()->SetIgnoreMoveInput(false);

	GetCharacterStatusComponent()->ClearAction();

	if (InteractActor->ActorHasTag("Ride"))
	{
		GetCharacterStatusComponent()->SetState(TAG_State_Ride);

		IInteractInterface::Execute_RegisterInteractActor(InteractActor, this);

		DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Ride = Cast<ARide>(InteractActor);
		CurRideStance = ERideStance::Mount;
	}
	else if (InteractActor->ActorHasTag("Ladder"))
	{
		
		bool IsRequestSuccess = ClimbComponent->RequestEnterLadder(InteractActor);
		if (!IsRequestSuccess) return;

		GetCharacterStatusComponent()->SetState(TAG_State_Ladder);
	}
	else if (InteractActor->ActorHasTag("NPC"))
	{
		IInteractInterface::Execute_Interact(InteractActor, this);
	}

	IsInteraction = true;
}

/* ============================================================
 *  Hit Reaction
 * ============================================================ */
void APlayerBase::OnHit_Implementation(const FAttackRequest& AttackInfo)
{
	float HitAngle = GetHitReactionComponent()->CalculateHitAngle(AttackInfo.HitPoint);

	EHitResponse Response = GetHitReactionComponent()->EvaluateHitResponse(AttackInfo);

	switch (Response)
	{
	case EHitResponse::NoStagger:
	{
		StatComponent->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
		StatComponent->ChangePoise(AttackInfo.PoiseDamage, EStatChangeType::Damage);
	}
	case EHitResponse::Flinch:
	case EHitResponse::KnockBack:
	case EHitResponse::KnockDown:
	{
		GetStatComponent()->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
		GetStatComponent()->ChangePoise(AttackInfo.PoiseDamage, EStatChangeType::Damage);
		if (GetStatComponent()->GetCommonStats().GetPoise() <= 0.0f && !GetCharacterStatusComponent()->IsDead())
		{
			FHitReactionRequest InputReaction = { Response, HitAngle };
			GetCharacterStatusComponent()->RequestAction(TAG_Action_HitReact);
			GetHitReactionComponent()->ExecuteHitResponse(InputReaction);
		}
		break;
	}
	case EHitResponse::HitAir:
	{
		StatComponent->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
		StatComponent->ChangePoise(AttackInfo.PoiseDamage, EStatChangeType::Damage);
		if (GetStatComponent()->GetCommonStats().GetPoise() <= 0.0f || GetCharacterStatusComponent()->IsDead())
		{
			CharacterBaseAnim->SetHitAir(true);
			GetCharacterStatusComponent()->RequestAction(TAG_Action_HitReact);
		}
		break;
	}
	case EHitResponse::Block:
	case EHitResponse::BlockLarge:
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
				GetCharacterStatusComponent()->RequestAction(TAG_Action_HitReact);
				GetHitReactionComponent()->ExecuteHitResponse(InputReaction);
			}
		}
		else
		{
			StatComponent->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
			Response = Response == EHitResponse::Block ? EHitResponse::BlockBreak : EHitResponse::BlockStun;
			FHitReactionRequest InputReaction = { Response, HitAngle };
			GetCharacterStatusComponent()->RequestAction(TAG_Action_HitReact);
			GetHitReactionComponent()->ExecuteHitResponse(InputReaction);
		}
		break;
	}
	}
}

void APlayerBase::HandleDeathStarted()
{
	// 입력 차단
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// 이전 State별 뒷정리
	const FGameplayTag PrevState = GetCharacterStatusComponent()->GetPreviousStateBeforeDeath();

	if (PrevState.MatchesTagExact(TAG_State_Ride))
	{
		// 탈것 위에서 사망 → 낙마(탈것 디스폰 + 컨트롤/콜리전 복구)
		IPlayerInterface::Execute_DespawnRide(this, GetVelocity());
	}
	else if (PrevState.MatchesTagExact(TAG_State_Ladder))
	{
		// 사다리에서 사망 → 사다리 디태치
		// TODO: ClimbComponent의 실제 탈출 API로 연결 (RequestEnterLadder의 짝)
		// 예: GetClimbComponent()->RequestExitLadder();
	}
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
void APlayerBase::MountEnd()
{
	FTransform MountTransform = IRideInterface::Execute_GetMountTransform(Ride);
	SetActorLocation(MountTransform.GetLocation());
	SetActorRotation(MountTransform.GetRotation().Rotator());

	if (GetWorldTimerManager().IsTimerActive(MountTimerHandle))
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetWorldTimerManager().ClearTimer(MountTimerHandle);
	}
	CurRideStance = ERideStance::Riding;
}

void APlayerBase::DespawnRide_Implementation(FVector InitVelocity)
{
	if (!Ride) return;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;

	if (Ride->GetClass()->ImplementsInterface(UViewDataInterface::StaticClass()))
	{
		FDetachmentTransformRules DetachmentRules = FDetachmentTransformRules(
			EDetachmentRule::KeepWorld, false);

		DetachFromActor(DetachmentRules);

		FTransform SpringArmTransform = IViewDataInterface::Execute_GetSpringArmTransform(Ride);
		float InitTargetArmLength = IViewDataInterface::Execute_GetTargetArmLength(Ride);

		SpringArm->TargetArmLength = InitTargetArmLength;
		SpringArm->SetWorldLocation(SpringArmTransform.GetLocation());
		SpringArm->SetWorldRotation(SpringArmTransform.GetRotation().Rotator());

		FRotator InitControllerRotator = IViewDataInterface::Execute_GetControllerRotation(Ride);

		UGameplayStatics::GetPlayerController(GetWorld(), 0)->Possess(this);
		UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetControlRotation(InitControllerRotator);
	}
	else
	{
		UGameplayStatics::GetPlayerController(GetWorld(), 0)->Possess(this);
	}

	CurRideStance = ERideStance::DisMount;
	FVector DisMountVelocity = InitVelocity * 0.4f;
	DisMountVelocity.Z = 600.0f;

	LaunchCharacter(DisMountVelocity, true, true);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}, 0.01f, false);

	GetWorldTimerManager().SetTimer(CameraSettingTimerHandle, this, &APlayerBase::CameraSettingTimer, 0.01f, true);
}

void APlayerBase::CameraSettingTimer()
{
	bool CheckTargetArmLength = false;

	float CurrentLength = SpringArm->TargetArmLength;
	float NewLength = FMath::FInterpTo(CurrentLength, 200.0f, GetWorld()->GetDeltaSeconds(), 1.0f);

	SpringArm->TargetArmLength = NewLength;

	if (FMath::IsNearlyEqual(NewLength, 200.0f, 1.0f))
	{
		SpringArm->TargetArmLength = 200.0f;
		CheckTargetArmLength = true;
	}

	bool CheckSpringArmLocation = false;

	FVector CurrentLocation = SpringArm->GetRelativeLocation();
	FVector NewLocation = FMath::VInterpTo(CurrentLocation, InitSpringArmLocation, GetWorld()->GetDeltaSeconds(), 1.0f);

	SpringArm->SetRelativeLocation(NewLocation);

	if (SpringArm->GetRelativeLocation().Equals(InitSpringArmLocation))
	{
		SpringArm->SetRelativeLocation(InitSpringArmLocation);
		CheckSpringArmLocation = true;
	}

	if (CheckTargetArmLength && CheckSpringArmLocation)
	{
		SpringArm->bEnableCameraLag = true;
		SpringArm->bEnableCameraRotationLag = true;
		GetWorld()->GetTimerManager().ClearTimer(CameraSettingTimerHandle);
	}
}

void APlayerBase::JumpDismountTimer()
{
}

void APlayerBase::MountTimer()
{
	FVector StartLocation = Ride->GetActorLocation();
	FVector TargetLocation = IRideInterface::Execute_GetMountTransform(Ride).GetLocation();

	FVector CurLocation = FMath::Lerp(StartLocation, TargetLocation, CharacterBaseAnim->GetCurveValue(FName("Char_Translation_Y")));
	CurLocation.Z = FMath::Lerp(StartLocation.Z, TargetLocation.Z, CharacterBaseAnim->GetCurveValue(FName("Char_Translation_Z")));

	SetActorLocation(CurLocation);
}

/* ============================================================
 *  View Data Interface
 * ============================================================ */
FTransform APlayerBase::GetCameraTransform_Implementation()
{
	return Camera->GetComponentTransform();
}

FTransform APlayerBase::GetSpringArmTransform_Implementation()
{
	return SpringArm->GetComponentTransform();
}

float APlayerBase::GetTargetArmLength_Implementation()
{
	return SpringArm->TargetArmLength;
}

FRotator APlayerBase::GetControllerRotation_Implementation()
{
	return GetController()->GetControlRotation();
}

TOptional<FVector> APlayerBase::GetCharBoneLocation(FName BoneName)
{
	return GetMesh()->DoesSocketExist(BoneName) ? TOptional<FVector>(GetMesh()->GetSocketLocation(BoneName)) : TOptional<FVector>();
}

void APlayerBase::DisMountEnd()
{
	GetCharacterStatusComponent()->SetState(TAG_State_Ground);
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

/* ============================================================
 *  LockOn
 * ============================================================ */
void APlayerBase::ApplyLockOnRotation(float DeltaTime)
{
	AActor* Target = LockOnComponent ? LockOnComponent->GetCurrentTarget() : nullptr;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!Target || !PC) return;

	const FVector MyLoc = GetActorLocation();
	const FVector TLoc = Target->GetActorLocation();

	FRotator Desired = (TLoc - MyLoc).Rotation();
	Desired.Pitch = 0.f;
	Desired.Roll = 0.f;

	FRotator Current = PC->GetControlRotation();
	FRotator NewRot = FMath::RInterpTo(Current, Desired, DeltaTime, LockOnTurnInterpSpeed);

	PC->SetControlRotation(NewRot);
}

void APlayerBase::SetLockOnMovementMode(bool bLockOn)
{
	bUseControllerRotationYaw = bLockOn;
	GetCharacterMovement()->bOrientRotationToMovement = !bLockOn;
	Jog();
}

void APlayerBase::OnLockOnToggle()
{
	UE_LOG(Log_LockOn, Warning, TEXT("[APlayerBase] %s Execute LockOn Toggle"), *GetName());
	if (LockOnComponent)
	{
		UE_LOG(Log_LockOn, Warning, TEXT("[APlayerBase] %s Access LockOn Allowed"), *GetName());
		const bool bLocked = LockOnComponent->ToggleLockOn();
		SetLockOnMovementMode(bLocked);
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
