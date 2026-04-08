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

// UI
#include "UI/DefaultWidget.h"
#include "Blueprint/UserWidget.h"

// 애니메이션
#include "Characters/Player/PlayerBaseAnimInstance.h"

// 참조할 액터
#include "Characters/Player/PlayerRide.h"

// 인터페이스
#include "Interaction/Interfaces/InteractInterface.h" ///삭제 예정
#include "Characters/Rideable/Interfaces/RideInterface.h"

// 유저 컴포넌트
#include "Characters/Components/BaseCharacterMovementComponent.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Combat/Components/CombatComponent.h"
#include "Combat/Components/PlayerAttackComponent.h"
#include "Combat/Components/PlayerHitReactionComponent.h"
#include "Interaction/Components/InteractComponent.h"
#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Characters/Player/Components/LockOnComponent.h"

// 데이터 참조
#include "Characters/Player/PlayerConfig.h"

//유틸리티
#include "Utils/CoreLog.h"

// Sets default values
APlayerBase::APlayerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
		.SetDefaultSubobjectClass<UPlayerAttackComponent>(TEXT("AttackComponent"))
		.SetDefaultSubobjectClass<UPlayerHitReactionComponent>(TEXT("HitReactionComponent"))
		.SetDefaultSubobjectClass<UPlayerStatusComponent>(TEXT("CharacterStatusComponent")))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StatComponent = CreateDefaultSubobject<UPlayerStatComponent>(TEXT("StatComponent"));
	StatComponent->bAutoActivate = true;

	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->bAutoActivate = true;

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
	//GetMesh()->SetOwnerNoSee(true);

	static ConstructorHelpers::FClassFinder<UDefaultWidget>Class_DefualtWidget(TEXT("/Game/00_Character/Data/DefaultWidget_BP"));
	if (Class_DefualtWidget.Succeeded()) DefaultWidgetClass = Class_DefualtWidget.Class;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> RollMontageAsset(TEXT("/Game/04_Animations/Player/SSH/Roll/Normal/Roll_Montage.Roll_Montage"));
	if (RollMontageAsset.Succeeded())
	{
		RollMontage = RollMontageAsset.Object;
	}

	GetMesh()->SetGenerateOverlapEvents(true);

	//CanMovementInput = true;
	CurLocomotionGait = ELocomotionGait::Jog;

	GetCharacterMovement()->MaxWalkSpeed = 450.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 800.0f;
	GetCharacterMovement()->MaxAcceleration = 800.0f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;
	GetCharacterMovement()->JumpZVelocity = 500.0f;
	GetCharacterMovement()->AirControl = 0.2f; // 1.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->GravityScale = 1.2f;
	GetCharacterMovement()->GroundFriction = 10.0f;
	//GetCharacterMovement()->MaxStepHeight

	EquipmentComponent->SetWeaponSocketName(FName("S_Sword"));
	EquipmentComponent->SetSubEquipSocketName(FName("S_SubEquip"));

	CameraSetting();
	CurrentProfileTag = FGameplayTag::RequestGameplayTag(FName("Skeleton.Player"));
	Tags.Add("Player");
}

// Called when the game starts or when spawned
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

	if (DefaultWidgetClass)
	{
		DefaultWidget = CreateWidget<UDefaultWidget>(PlayerController, DefaultWidgetClass);
	}

	//StatComponent->OnDeath.BindUObject(this, &APlayerBase::Death);
	StatComponent->InitializeStats();
	GetAttackComponent()->SetCurAttackContextSet(EWeaponType::SwordAndShield);

	InitSpringArmLocation = SpringArm->GetRelativeLocation();
}

// Called every frame
void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bForcedRotatingInputDirection)
	{
		FRotator CurrentRot = GetActorRotation();
		FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, InputRotation, DeltaTime, ForcedRotationSpeed);

		// 일정 각도 이하로 차이 나면 고정
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

	FVector LastInputDirection = GetLastMovementInputVector().GetSafeNormal();
	if (!LastInputDirection.IsNearlyZero())
	{
		FVector MovementDirection = GetVelocity().GetSafeNormal();
		//GetActorForwardVector();
	// 입력 방향 (Movement Input)
	// 캐릭터 위치
		FVector DebugStartLocation = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		// 디버깅 시작위치
		float DotProduct = FVector::DotProduct(MovementDirection, LastInputDirection);
		// 아크코사인(Arccos)을 이용해 각도 구하기 (라디안)
		float RadianAngle = FMath::Acos(DotProduct);
		// 라디안을 각도로 변환
		float DegreeAngle = FMath::RadiansToDegrees(RadianAngle);

		FNumberFormattingOptions FormatOptions;
		FormatOptions.SetMaximumFractionalDigits(1); // 보기 좋게 1자리

		FText DebugAxisText = FText::AsNumber(DegreeAngle, &FormatOptions);
		FString DebugAxisString = DebugAxisText.ToString();
		//FString::Append(DebugAxisText);

	// 디버깅용 길이
		float DebugLineLength = 100.0f;

		// 이동 방향 표시
		DrawDebugDirectionalArrow(
			GetWorld(),
			DebugStartLocation,
			DebugStartLocation + MovementDirection * DebugLineLength,
			50.0f,          // 화살표 크기
			FColor::Green,   // 이동 방향은 녹색
			false,           // 영구 표시 여부 (false: 짧은 시간만 표시)
			0.0f,            // 표시 시간
			0,               // 두께
			2.0f             // 선 두께
		);

		// 입력 방향 표시
		DrawDebugDirectionalArrow(
			GetWorld(),
			DebugStartLocation,
			DebugStartLocation + LastInputDirection * DebugLineLength,
			50.0f,
			FColor::Blue,    // 입력 방향은 파란색
			false,
			0.0f,
			0,
			2.0f
		);

		DrawDebugString(
			GetWorld(),
			DebugStartLocation,
			DebugAxisString,
			0,
			FColor::White,
			0.0f
		);
	}
}

void APlayerBase::ApplyConfig()
{
	if (!Config) { ensureMsgf(false, TEXT("Config missing")); return; }

	GetCharacterStatusComponent()->WindowRules = Config->WindowRules;
	GetMesh()->SetSkeletalMesh(Config->Mesh);
	GetMesh()->SetAnimInstanceClass(Config->AnimBP);
	HitReactionComponent->SetHitReactionDA(Config->HitReactData);
	AttackComponent->SetAttackDA(Config->AttackData);
}

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

// Called to bind functionality to input
void APlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InputConfig->Move, ETriggerEvent::Triggered, this, &APlayerBase::Move);
		EnhancedInputComponent->BindAction(InputConfig->CheckMove, ETriggerEvent::Started, this, &APlayerBase::StartMoveInput);
		EnhancedInputComponent->BindAction(InputConfig->CheckMove, ETriggerEvent::Completed, this, &APlayerBase::EndMoveInput);

		EnhancedInputComponent->BindAction(InputConfig->Look, ETriggerEvent::Triggered, this, &APlayerBase::Look);

		EnhancedInputComponent->BindAction(InputConfig->Jump, ETriggerEvent::Triggered, this, &APlayerBase::Jump);

		EnhancedInputComponent->BindAction(InputConfig->Block, ETriggerEvent::Ongoing, this, &APlayerBase::OnBlock);
		EnhancedInputComponent->BindAction(InputConfig->Block, ETriggerEvent::Triggered, this, &APlayerBase::OffBlock);

		EnhancedInputComponent->BindAction(InputConfig->Parry, ETriggerEvent::Triggered, this, &APlayerBase::Parry);

		EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Triggered, this, &APlayerBase::Dodge);

		EnhancedInputComponent->BindAction(InputConfig->Interact, ETriggerEvent::Triggered, this, &APlayerBase::Interact);

		EnhancedInputComponent->BindAction(InputConfig->Sprint, ETriggerEvent::Started, this, &APlayerBase::Sprint);
		EnhancedInputComponent->BindAction(InputConfig->Sprint, ETriggerEvent::Triggered, this, &APlayerBase::Jog);

		EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Started, this, &APlayerBase::Walk);
		EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Triggered, this, &APlayerBase::Jog);

		EnhancedInputComponent->BindAction(InputConfig->SpawnRide, ETriggerEvent::Triggered, this, &APlayerBase::SpawnRide);

		EnhancedInputComponent->BindAction(InputConfig->Attack, ETriggerEvent::Started, this, &APlayerBase::AttackInput);
		EnhancedInputComponent->BindAction(InputConfig->Attack, ETriggerEvent::Triggered, this, &APlayerBase::AttackInputEnd);

		EnhancedInputComponent->BindAction(InputConfig->Modifier, ETriggerEvent::Ongoing, this, &APlayerBase::ModifierInput);
		EnhancedInputComponent->BindAction(InputConfig->Modifier, ETriggerEvent::Triggered, this, &APlayerBase::ModifierInputEnd);

		EnhancedInputComponent->BindAction(InputConfig->LockOn, ETriggerEvent::Triggered, this, &APlayerBase::OnLockOnToggle);
		EnhancedInputComponent->BindAction(InputConfig->LockOnSwitchLeft, ETriggerEvent::Triggered, this, &APlayerBase::OnLockOnSwitchLeft);
		EnhancedInputComponent->BindAction(InputConfig->LockOnSwitchRight, ETriggerEvent::Triggered, this, &APlayerBase::OnLockOnSwitchRight);
	}
}

void APlayerBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InteractComponent->OnArrivedInteractionPoint.BindUObject(this, &APlayerBase::HandleArrivedInteractionPoint);

	if (GetCharacterStatusComponent())
	{
		GetCharacterStatusComponent()->OnDeath.AddUObject(this, &APlayerBase::OnDeath);
	}
}

/* Input Action */
void APlayerBase::Move(const FInputActionValue& value)
{
	IsMovementInput = true;

	const FVector2D DirectionValue = value.Get<FVector2D>();

	switch (GetCharacterStatusComponent()->GetCharacterState_Native())
	{
	case ECharacterState::Ground:
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
		break;
	}
	case ECharacterState::Ladder:
	{
		IsMovementInput = FMath::IsNearlyZero(DirectionValue.Y) ? false : true;
		if (!IsMovementInput) return;
		
		DirectionValue.Y > 0.0f ? GetClimbComponent()->ClimbUpLadder() : GetClimbComponent()->ClimbDownLadder();
		break;

	}
	default:
	{
		break;
	}
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

void APlayerBase::Dodge()
{
	if (CharacterBaseAnim->GetCurveValue(FName("EnableDodge")) < 0.9f) return;

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

	CharacterStatusComponent->SetGroundStance_Native(EGroundStance::Dodge);
}

void APlayerBase::Jump()
{
	if (CharacterBaseAnim->GetCurveValue(FName("EnableJump")) < 0.9f)
		return;

	if (CharacterBaseAnim->GetCurrentActiveMontage())
	{
		CharacterBaseAnim->Montage_Stop(0.1f);
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &Super::Jump);
	}
	else
	{
		GetCharacterStatusComponent()->SetGroundStance_Native(EGroundStance::Jump);
		Super::Jump();
	}
	//GetCharacterMovement()->bOrientRotationToMovement = false;
}

void APlayerBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	GetCharacterStatusComponent()->SetGroundStance_Native(EGroundStance::Normal);
	GetCharacterMovement()->bOrientRotationToMovement = true;

	const bool bNoMoveInput = false;
	//GetInputAxisValue("MoveAction") == 0.f;

	if (bNoMoveInput)
	{
		FVector V = GetCharacterMovement()->Velocity;
		V.X *= 0.1f;
		V.Y *= 0.1f;
		GetCharacterMovement()->Velocity = V;
	}
}

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

void APlayerBase::Interact()
{
	if (IsInteraction)
	{
		//bUseControllerRotationYaw = true;
		//CurrentState = ECharacterState::Ground;
		GetController()->SetIgnoreMoveInput(false);
		GetController()->StopMovement();
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		IsInteraction = false;
		return;
	}
	else
	{
		bool InteractTargetValid = InteractComponent->SetInteractActorByDegree(this, 60.0f);

		if (!InteractTargetValid)
			return;

		GetController()->SetIgnoreMoveInput(true);
		IsInteraction = InteractComponent->MovetoInteractPos();
	}
}

void APlayerBase::MountEnd()
{
	//IInteractInterface::Execute_Interact(Ride, this);

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

void APlayerBase::Attack(FName AttackName)
{
	GetAttackComponent()->ExecuteAttack(AttackName);
}

void APlayerBase::AttackInput()
{
	if (CharacterBaseAnim->GetCurveValue(FName("EnableAttack")) < 0.9f) return;

	IsAttackInput = true;

	GetAttackComponent()->ExecuteAttack(FName("DefaultCombo"));
}

void APlayerBase::ChargeAttackTimer()
{
	AttackChargeTime += 0.1f;
	UE_LOG(LogTemp, Warning, TEXT("AttackTimer : %f"), AttackChargeTime);
	if (AttackChargeTime >= 1.0f || !IsModifierInput || !IsAttackInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reset Timer"));
		GetAttackComponent()->ExecuteAttack(FName("ChargeAttack"), 2.0f - AttackChargeTime);
		AttackChargeTime = 0.0f;
		GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	}
}

void APlayerBase::AttackInputEnd()
{
	IsAttackInput = false;
	//UE_LOG(LogTemp, Warning, TEXT("inputend"));
}

bool APlayerBase::GetIsMovementInput()
{
	return IsMovementInput;
}

float APlayerBase::GetRideSpeed()
{
	if (Ride == nullptr)
		return 0.0f;

	return IRideInterface::Execute_GetRideSpeed(Ride);
}

float APlayerBase::GetRideDirection()
{
	if (Ride == nullptr)
		return 0.0f;

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
	{
		return Ride->GetMesh()->DoesSocketExist(FName("Reins_Bn_Hand_L"))
			? Ride->GetMesh()->GetSocketLocation(FName("Reins_Bn_Hand_L"))
			: TOptional<FVector>();
	}
	case EBodyType::Hand_R:
	{
		return Ride->GetMesh()->DoesSocketExist(FName("Reins_Bn_Hand_R"))
			? Ride->GetMesh()->GetSocketLocation(FName("Reins_Bn_Hand_R"))
			: TOptional<FVector>();
	}
	case EBodyType::Foot_L:
	{
		return Ride->GetMesh()->DoesSocketExist(FName("SaddleLeftFootPlace"))
			? Ride->GetMesh()->GetSocketLocation(FName("SaddleLeftFootPlace"))
			: TOptional<FVector>();
	}
	case EBodyType::Foot_R:
	{
		return Ride->GetMesh()->DoesSocketExist(FName("SaddleRightFootPlace"))
			? Ride->GetMesh()->GetSocketLocation(FName("SaddleRightFootPlace"))
			: TOptional<FVector>();
	}
	default:
	{
		return TOptional<FVector>();
	}
	}
}

bool APlayerBase::IsPlayer_Implementation()
{
	IPlayerInterface::IsPlayer_Implementation();

	return false;
}

void APlayerBase::RegisterInteractableActor_Implementation(AActor* Interactable)
{
	IPlayerInterface::RegisterInteractableActor_Implementation(Interactable);

	InteractComponent->AddInteractObject(Interactable);
	//InteractActorList.Add(Interactable);
}

void APlayerBase::DeRegisterInteractableActor_Implementation(AActor* Interactable)
{
	IPlayerInterface::DeRegisterInteractableActor_Implementation(Interactable);

	InteractComponent->RemoveInteractObject(Interactable);
}

void APlayerBase::EndInteraction_Implementation(AActor* Interactable)
{
	IPlayerInterface::EndInteraction_Implementation(Interactable);

	if (Interactable->ActorHasTag("Ride"))
	{
		//IsRide = false;

		//GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	}
}

void APlayerBase::HandleArrivedInteractionPoint()
{
	AActor* InteractActor = InteractComponent->GetInteractActor();
	USceneComponent* InteractionPoint = IInteractInterface::Execute_GetEnterInteractLocation(InteractActor, this);

	GetController()->SetIgnoreMoveInput(false);

	if (InteractActor->ActorHasTag("Ride"))
	{
		IInteractInterface::Execute_RegisterInteractActor(InteractActor, this);

		DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Ride = Cast<ACharacter>(InteractActor);
		CurRideStance = ERideStance::Mount;
		GetCharacterStatusComponent()->SetCharacterState_Native(ECharacterState::Ride);
	}
	else if (InteractActor->ActorHasTag("Ladder"))
	{
		bool IsRequestSuccess = ClimbComponent->RequestEnterLadder(InteractActor);
	}
	else if (InteractActor->ActorHasTag("NPC"))
	{
		IInteractInterface::Execute_Interact(InteractActor, this);
	}

	IsInteraction = true;
}

void APlayerBase::OnBlock()
{
	if (CharacterBaseAnim->GetCurveValue(FName("EnableBlock")) < 0.9f) return;
	IsBlockInput = true;
	GetCharacterStatusComponent()->SetGroundStance_Native(EGroundStance::Block);
}

void APlayerBase::OffBlock()
{
	IsBlockInput = false;
	GetCharacterStatusComponent()->SetGroundStance_Native(EGroundStance::Normal);
}

void APlayerBase::Parry()
{
	//CharacterStatusComponent
}

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
			FHitReactionRequest InputReaction = { Response,HitAngle };
			GetCharacterStatusComponent()->SetGroundStance_Native(EGroundStance::Hit);
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
			GetCharacterStatusComponent()->SetGroundStance_Native(EGroundStance::Hit);
		}
		break;
	}
	case EHitResponse::Block:
	case EHitResponse::BlockLarge:
	{
		float PerformanceRatio = IStatInterface::Execute_GetWeaponPerformanceRatio(StatComponent, EquipmentComponent->GetEquipedWeapon()->RequiredStats.ToCharacterStats());
		float GuardBoost = EquipmentComponent->GetEquipedWeapon()->GuardBoost;
		float GuardNegation = EquipmentComponent->GetEquipedWeapon()->GuardNegation;
		if (PerformanceRatio < 1.0f)
		{
			GuardBoost *= 0.8f;
			GuardNegation *= 0.8f;
		}

		float ApplyGuardBoost = AttackInfo.StanceDamage * (1.0f - GuardBoost / 100.0f);
		bool IsStaminaEnough = StatComponent->ChangeStamina(ApplyGuardBoost, EStatChangeType::Damage);
		if (IsStaminaEnough)
		{
			float ApplyNegationDamage = AttackInfo.Damage * (1.0f - GuardNegation / 100.0f);
			StatComponent->ApplyDamage(ApplyNegationDamage, AttackInfo.AttackType);
			if (!GetCharacterStatusComponent()->IsDead())
			{
				FHitReactionRequest InputReaction = { Response, HitAngle };
				GetHitReactionComponent()->ExecuteHitResponse(InputReaction);
			}
		}
		else
		{
			StatComponent->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
			Response = Response == EHitResponse::Block ? EHitResponse::BlockBreak : EHitResponse::BlockStun;
		}
		break;
	}
	}
}

void APlayerBase::OnDeathEnd_Implementation()
{
	// 사망 시 UI 호출
	// 스탯 초기화
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true); // 일단 꺼줌
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 사망효과로 교체
	SetLifeSpan(5.0f);
}

void APlayerBase::OnDeath()
{
	// 기본 UI 비활성화
	APlayerController* MyController = Cast<APlayerController>(GetController());
	DisableInput(MyController);
}

void APlayerBase::SpawnRide()
{
	if (CharacterBaseAnim->GetCurveValue(FName("EnableRide")) < 0.9f)
	{
		UE_LOG(Log_RideSpawn, Warning, TEXT("[APlayerBase] %s : Cant Spawn Ride"), *GetName());
		return;
	}

	Ride = GetWorld()->SpawnActor<APlayerRide>(GetActorLocation(), GetActorRotation());
	if (!Ride)
	{
		UE_LOG(Log_RideSpawn, Warning, TEXT("[APlayerBase] %s : Horse was Not Spawned"), *GetName());
		return;
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	IRideInterface::Execute_Mount(Ride, this, GetVelocity());

	GetCharacterMovement()->SetMovementMode(MOVE_None);

	CurRideStance = ERideStance::Mount;
	GetCharacterStatusComponent()->SetCharacterState_Native(ECharacterState::Ride);

	GetWorldTimerManager().SetTimer(MountTimerHandle, this, &APlayerBase::MountTimer, 0.01f, true);
}

void APlayerBase::DespawnRide_Implementation(FVector InitVelocity)
{
	if (!Ride)
	{
		return;
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;

	if (Ride->GetClass()->ImplementsInterface(UViewDataInterface::StaticClass()))
	{
		FDetachmentTransformRules DetachmentRules = FDetachmentTransformRules(
			EDetachmentRule::KeepWorld,
			false
		);

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
	//GetCharacterMovement()->SetMovementMode(MOVE_Walking);
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
	UE_LOG(LogTemp, Warning, TEXT("Dismountend"));
	GetCharacterStatusComponent()->SetCharacterState_Native(ECharacterState::Ground);
}

UPlayerAttackComponent* APlayerBase::GetAttackComponent() const
{
	return Cast<UPlayerAttackComponent>(AttackComponent);
}

UPlayerHitReactionComponent* APlayerBase::GetHitReactionComponent() const
{
	return Cast<UPlayerHitReactionComponent>(HitReactionComponent);
}

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
	if(LockOnComponent) LockOnComponent->SwitchTarget(false);
}

void APlayerBase::OnLockOnSwitchRight()
{
	if (LockOnComponent) LockOnComponent->SwitchTarget(true);
}

UPlayerStatusComponent* APlayerBase::GetCharacterStatusComponent() const
{
	return Cast<UPlayerStatusComponent>(CharacterStatusComponent);
}