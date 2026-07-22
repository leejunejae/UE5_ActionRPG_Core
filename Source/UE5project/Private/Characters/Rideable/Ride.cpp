// Fill out your copyright notice in the Description page of Project Settings.

// 기본 헤더
#include "Characters/Rideable/Ride.h"

// 카메라
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

// 이동
#include "GameFramework/CharacterMovementComponent.h"

// 콜리전
#include "Components/CapsuleComponent.h"

// 입력
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

// Kismet 유틸리티
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetTextLibrary.h"

// 애니메이션
#include "Characters/Rideable/RideAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Curves/CurveFloat.h"

// 컴포넌트
#include "Characters/Components/RideComponent.h"

// Sets default values
ARide::ARide()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RideTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ride.Horse")));

	RootComponent = GetCapsuleComponent();
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	Camera->SetupAttachment(SpringArm);
	Camera->SetConstraintAspectRatio(false);

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Ride"));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh>HORSE_MESH(TEXT("/Game/05_Ride/DefaultHorse/Mesh/SK_Horse.SK_Horse"));
	if (HORSE_MESH.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(HORSE_MESH.Object);
	}
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeLocation(FVector(-50.0f, 0.0f, -90.0f));

	Saddle = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Saddle"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SADDLE(TEXT("/Game/05_Ride/DefaultHorse/Mesh/SK_Horse_Saddle.SK_Horse_Saddle"));
	if (SADDLE.Succeeded())
	{
		Saddle->SetSkeletalMesh(SADDLE.Object);
	}
	Saddle->SetupAttachment(GetMesh());
	Saddle->SetLeaderPoseComponent(GetMesh());

	Reins = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Reins"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> REINS(TEXT("/Game/05_Ride/DefaultHorse/Mesh/SK_Horse_Reins.SK_Horse_Reins"));
	if (REINS.Succeeded())
	{
		Reins->SetSkeletalMesh(REINS.Object);
	}
	Reins->SetupAttachment(GetMesh());
	Reins->SetLeaderPoseComponent(GetMesh());

	FName MountSocket(TEXT("MountPoint"));
	RiderLocation = CreateDefaultSubobject<USceneComponent>(TEXT("RiderLocation"));
	RiderLocation->SetupAttachment(RootComponent, MountSocket);
	RiderLocation->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	//RiderLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	RiderGetDownLoc = CreateDefaultSubobject<USceneComponent>(TEXT("RiderGetDownLoc"));
	RiderGetDownLoc->SetupAttachment(GetMesh());
	RiderGetDownLoc->SetRelativeLocation(FVector(0.0f, -110.0f, -90.0f));

	RiderMountLocLeft = CreateDefaultSubobject<USceneComponent>(TEXT("RiderMountLocLeft"));
	RiderMountLocLeft->SetupAttachment(GetMesh());
	RiderMountLocLeft->SetRelativeLocation(FVector(70.0f, 44.0f, 85.0f));
	RiderMountLocLeft->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	RiderMountLocRight = CreateDefaultSubobject<USceneComponent>(TEXT("RiderMountLocRight"));
	RiderMountLocRight->SetupAttachment(GetMesh());
	RiderMountLocRight->SetRelativeLocation(FVector(-70.0f, 44.0f, 85.0f));
	RiderMountLocRight->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f,0.0f,90.0f));
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
	SpringArm->SocketOffset = FVector(0.0f, 60.0f, 0.0f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagSpeed = 10.0f;
	SpringArm->CameraRotationLagSpeed = 10.0f;

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->MaxWalkSpeed = MaxRideSpeed;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 120.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->BrakingDecelerationWalking = 300.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 2.0f;

	GetCharacterMovement()->bRunPhysicsWithNoController = true;

	InputSetting();

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	
	static ConstructorHelpers::FClassFinder<UAnimInstance>HORSE_ANIM(TEXT("/Game/05_Ride/AnimData/RHAB_AnimBlueprint.RHAB_AnimBlueprint_C"));
	if (HORSE_ANIM.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(HORSE_ANIM.Class);
	}

	Tags.Add("Ride");
}

void ARide::InputSetting()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>PR_Context(TEXT("/Game/00_Character/C_Input/R_BasicInput.R_BasicInput"));
	if (PR_Context.Succeeded())
	{
		DefaultContext = PR_Context.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>IP_Move(TEXT("/Game/00_Character/C_Input/C_Move.C_Move"));
	if (IP_Move.Succeeded())
	{
		MoveAction = IP_Move.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>IP_Look(TEXT("/Game/00_Character/C_Input/C_Look.C_Look"));
	if (IP_Look.Succeeded())
	{
		LookAction = IP_Look.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>IP_DisMount(TEXT("/Game/00_Character/C_Input/C_SpawnRide.C_SpawnRide"));
	if (IP_DisMount.Succeeded())
	{
		DisMountAction = IP_DisMount.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>IP_Walk(TEXT("/Game/00_Character/C_Input/C_Walk.C_Walk"));
	if (IP_Walk.Succeeded())
	{
		WalkAction = IP_Walk.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction>IP_Sprint(TEXT("/Game/00_Character/C_Input/C_Sprint.C_Sprint"));
	if (IP_Sprint.Succeeded())
	{
		SprintAction = IP_Sprint.Object;
	}
}

// Called when the game starts or when spawned
void ARide::BeginPlay()
{
	Super::BeginPlay();

	MaxRideSpeed = FMath::Max(WalkRideSpeed, FMath::Max(RunRideSpeed, SprintRideSpeed));
	GetCharacterMovement()->MaxWalkSpeed = MaxRideSpeed;
	
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			SubSystem->AddMappingContext(DefaultContext, 0);
		}
	}
}

// Called every frame
void ARide::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDismount)
	{
		AddMovementInput(GetActorForwardVector(), 1.0f, true);
		Direction = FMath::FInterpTo(Direction, 0.0f, DeltaTime, 5.0f);
	}
	else
	{
		UpdateRideMovement(DeltaTime);
	}


	FRotator SocketRot = GetMesh()->GetSocketRotation(FName("MountPoint"));
	FRotator CurrentRot = RiderLocation->GetComponentRotation();

	// 느린 보간 → 큰 회전만 부드럽게 따라감, 미세 흔들림은 무시됨
	float InterpSpeed = 5.0f; // 낮을수록 더 부드럽게, 높을수록 빠르게 추종
	FRotator SmoothedRot = FMath::RInterpTo(CurrentRot, SocketRot, DeltaTime, InterpSpeed);

	RiderLocation->SetWorldRotation(SmoothedRot);
	RiderLocation->SetWorldLocation(GetMesh()->GetSocketLocation(FName("MountPoint")));
}

// Called to bind functionality to input
void ARide::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARide::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ARide::StopMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ARide::StopMove);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARide::Look);
		EnhancedInputComponent->BindAction(DisMountAction, ETriggerEvent::Triggered, this, &ARide::DisMount);

		if (WalkAction)
		{
			EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &ARide::StartWalk);
			EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Completed, this, &ARide::StopWalk);
			EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Canceled, this, &ARide::StopWalk);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ARide::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ARide::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ARide::StopSprint);
		}
	}
}

void ARide::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

float ARide::GetDirection() const
{
	return Direction;
}

void ARide::Move(const FInputActionValue& value)
{
	RideMoveInput = value.Get<FVector2D>();
}

void ARide::StopMove(const FInputActionValue& value)
{
	RideMoveInput = FVector2D::ZeroVector;
}

void ARide::StartWalk(const FInputActionValue& value)
{
	bWantsWalk = true;
}

void ARide::StopWalk(const FInputActionValue& value)
{
	bWantsWalk = false;
}

void ARide::StartSprint(const FInputActionValue& value)
{
	bWantsSprint = true;
}

void ARide::StopSprint(const FInputActionValue& value)
{
	bWantsSprint = false;
}

void ARide::UpdateRideMovement(float DeltaTime)
{
	if (bPivotTurning)
	{
		UpdatePivotTurn(DeltaTime);
		return;
	}

	FVector2D RawInput = RideMoveInput;
	if (RawInput.SizeSquared() > 1.0f)
	{
		RawInput.Normalize();
	}

	const bool bHasMoveInput = RawInput.SizeSquared() > FMath::Square(InputDeadZone);

	float TargetThrottle = 0.0f;
	float TargetDirection = 0.0f;
	const float PreviousYaw = GetActorRotation().Yaw;

	if (bHasMoveInput)
	{
		const ERideGait DesiredGait = GetDesiredRideGait(RawInput);
		const float TargetSpeed = GetRideSpeedForGait(DesiredGait);
		TargetThrottle = FMath::Clamp(TargetSpeed / FMath::Max(MaxRideSpeed, 1.0f), 0.0f, 1.0f);

		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		FVector2D MovementScale = RawInput;
		MovementScale.Normalize();

		FVector MovementDirection = GetActorForwardVector();
		MovementDirection.Z = 0.0f;
		MovementDirection.Normalize();

		FVector LastInputDirection = (UKismetMathLibrary::GetForwardVector(YawRotation) * MovementScale.Y) + (UKismetMathLibrary::GetRightVector(YawRotation) * MovementScale.X);
		LastInputDirection.Z = 0.0f;
		LastInputDirection.Normalize();

		float DotProductDirection = FMath::Clamp(FVector::DotProduct(MovementDirection, LastInputDirection), -1.0f, 1.0f);
		float DotProductRadian = FMath::Acos(DotProductDirection);
		float DotProductDegree = FMath::RadiansToDegrees(DotProductRadian);

		FVector RotationAxis = FVector::CrossProduct(MovementDirection, LastInputDirection);
		if (!RotationAxis.Normalize())
		{
			RotationAxis = RawInput.X >= 0.0f ? FVector::UpVector : -FVector::UpVector;
		}

		TargetDirection = RotationAxis.Z > 0.0f ? DotProductDegree : -1.0f * DotProductDegree;
		TargetDirection = FMath::Clamp(TargetDirection, -MaxAnimDirection, MaxAnimDirection);

		if (DotProductDegree > PivotTurnMinAngle)
		{
			if (CanStartPivotTurn(DotProductDegree))
			{
				const float DesiredYaw = LastInputDirection.Rotation().Yaw;
				const float TargetDeltaYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredYaw);
				PivotTurn(TargetDeltaYaw);
				return;
			}

			TargetThrottle = 0.0f;
		}

		const float SpeedAlpha = FMath::Clamp(GetVelocity().Size2D() / FMath::Max(MaxRideSpeed, 1.0f), 0.0f, 1.0f);
		const float TurnRateBySpeed = FMath::Lerp(MaxTurnRate, MinTurnRateAtMaxSpeed, SpeedAlpha);
		const float Steering = FMath::Clamp(TargetDirection / 90.0f, -1.0f, 1.0f);
		AddActorWorldRotation(FRotator(0.0f, Steering * TurnRateBySpeed * DeltaTime, 0.0f));
	}

	const float InterpSpeed = TargetThrottle > CurrentThrottle ? AccelerationInterpSpeed : DecelerationInterpSpeed;
	CurrentThrottle = FMath::FInterpTo(CurrentThrottle, TargetThrottle, DeltaTime, InterpSpeed);
	Direction = FMath::FInterpConstantTo(Direction, TargetDirection, DeltaTime, DirectionInterpRate);

	if (CurrentThrottle > KINDA_SMALL_NUMBER)
	{
		AddMovementInput(GetActorForwardVector(), CurrentThrottle);
	}

	const float CurrentYaw = GetActorRotation().Yaw;
	TurnRate = DeltaTime > KINDA_SMALL_NUMBER
		? FMath::FindDeltaAngleDegrees(PreviousYaw, CurrentYaw) / DeltaTime
		: 0.0f;

	const float Speed2D = GetVelocity().Size2D();
	bBraking = !bHasMoveInput && Speed2D > WalkRideSpeed * 0.5f;

	if (Speed2D < KINDA_SMALL_NUMBER)
	{
		CurrentGait = ERideGait::Idle;
	}
	else if (Speed2D < (WalkRideSpeed + RunRideSpeed) * 0.5f)
	{
		CurrentGait = ERideGait::Walk;
	}
	else if (Speed2D < (RunRideSpeed + SprintRideSpeed) * 0.5f)
	{
		CurrentGait = ERideGait::Run;
	}
	else
	{
		CurrentGait = ERideGait::Sprint;
	}
}

void ARide::UpdatePivotTurn(float DeltaTime)
{
	if (!PivotTurnMontage)
	{
		FinishPivotTurn();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		if (!AnimInstance->Montage_IsPlaying(PivotTurnMontage))
		{
			FinishPivotTurn();
			return;
		}
	}
	else
	{
		FinishPivotTurn();
		return;
	}

	const float Alpha = GetPivotTurnCurveAlpha(AnimInstance);
	const float TargetYaw = PivotTurnStartYaw + (PivotTurnTargetDeltaYaw * Alpha);

	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw = TargetYaw;
	SetActorRotation(NewRotation);

	TurnRate = DeltaTime > KINDA_SMALL_NUMBER
		? FMath::FindDeltaAngleDegrees(PivotTurnPreviousYaw, TargetYaw) / DeltaTime
		: 0.0f;
	PivotTurnPreviousYaw = TargetYaw;

	CurrentThrottle = 0.0f;
	Direction = PivotTurnDirection * MaxAnimDirection;
	bBraking = false;
	CurrentGait = ERideGait::Idle;
}

bool ARide::CanStartPivotTurn(float DotProductDegree) const
{
	return !bPivotTurning
		&& DotProductDegree > PivotTurnMinAngle
		&& GetVelocity().Size2D() <= PivotTurnMaxStartSpeed;
}

float ARide::GetRideSpeedForGait(ERideGait Gait) const
{
	switch (Gait)
	{
	case ERideGait::Walk:
		return WalkRideSpeed;
	case ERideGait::Run:
		return RunRideSpeed;
	case ERideGait::Sprint:
		return SprintRideSpeed;
	case ERideGait::Idle:
	default:
		return 0.0f;
	}
}

ERideGait ARide::GetDesiredRideGait(const FVector2D& MoveInput) const
{
	if (MoveInput.SizeSquared() <= FMath::Square(InputDeadZone))
		return ERideGait::Idle;

	if (bWantsWalk || MoveInput.Size() < WalkInputThreshold)
		return ERideGait::Walk;

	if (bWantsSprint && MoveInput.Y > 0.0f)
		return ERideGait::Sprint;

	return ERideGait::Run;
}

float ARide::GetPivotTurnCurveAlpha(UAnimInstance* AnimInstance) const
{
	if (!AnimInstance || !PivotTurnMontage)
		return 0.0f;

	const float MontagePosition = AnimInstance->Montage_GetPosition(PivotTurnMontage);
	const FName CurrentSection = AnimInstance->Montage_GetCurrentSection(PivotTurnMontage);
	const int32 SectionIndex = PivotTurnMontage->GetSectionIndex(CurrentSection);

	float SectionStartTime = 0.0f;
	float SectionEndTime = PivotTurnMontage->GetPlayLength();
	if (SectionIndex != INDEX_NONE)
	{
		PivotTurnMontage->GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);
	}

	const float SectionLength = FMath::Max(SectionEndTime - SectionStartTime, KINDA_SMALL_NUMBER);
	const float SectionTime = FMath::Clamp(MontagePosition - SectionStartTime, 0.0f, SectionLength);
	const float SectionAlpha = FMath::Clamp((MontagePosition - SectionStartTime) / SectionLength, 0.0f, 1.0f);
	const float CurveTime = bUseNormalizedPivotTurnCurveTime ? SectionAlpha : SectionTime;
	const float CurveAlpha = PivotTurnAlphaCurve ? PivotTurnAlphaCurve->GetFloatValue(CurveTime) : SectionAlpha;

	return FMath::Clamp(CurveAlpha, 0.0f, 1.0f);
}

void ARide::FinishPivotTurn()
{
	if (!bPivotTurning)
		return;

	const float FinalYaw = PivotTurnStartYaw + PivotTurnTargetDeltaYaw;

	FRotator FinalRotation = GetActorRotation();
	FinalRotation.Yaw = FinalYaw;
	SetActorRotation(FinalRotation);

	bPivotTurning = false;
	PivotTurnDirection = 0.0f;
	PivotTurnTargetDeltaYaw = 0.0f;
	Direction = 0.0f;
	TurnRate = 0.0f;
}

void ARide::Look(const FInputActionValue& value)
{
	const FVector2D LookAxisValue = value.Get<FVector2D>();
	AddControllerPitchInput(LookAxisValue.Y * 0.5f);
	AddControllerYawInput(LookAxisValue.X * -0.5f);
}

void ARide::Mount(ACharacter* RiderCharacter, FVector InitVelocity)
{
	if (!RiderCharacter)
		return;

	Rider = RiderCharacter;
	GetCharacterMovement()->Velocity = InitVelocity;

	CanDismount = false;
	bDismount = false;
	bMovingDismount = false;
}

void ARide::AttachRider()
{
	if (!Rider)
		return;

	FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld,
		EAttachmentRule::KeepRelative,
		true
	);

	Rider->AttachToComponent(RiderLocation, AttachmentRules);
}

void ARide::DisMount()
{
	TryDisMount();
}

bool ARide::TryDisMount()
{
	if (!Rider || !RiderGetDownLoc || !CanDismount)
		return false;

	if (URideComponent* RideComponent = Rider->FindComponentByClass<URideComponent>())
	{
		LastSpeed = GetVelocity();
		bMovingDismount = LastSpeed.SizeSquared2D() > FMath::Square(MovingDismountSpeedThreshold);

		if (!RideComponent->RequestDismount(GetVelocity()))
		{
			return false;
		}

		bDismount = bMovingDismount;
		Rider = nullptr;
	}

	return true;
}

void ARide::FinishDismount()
{
	Destroy();
}

bool ARide::IsMovingDismount() const
{
	return bMovingDismount;
}

bool ARide::FindMountPos()
{
	FVector DistRightLoc = Rider->GetActorLocation() - RiderMountLocRight->GetComponentLocation();
	FVector DistLeftLoc = Rider->GetActorLocation() - RiderMountLocLeft->GetComponentLocation();

	return DistRightLoc.Length() < DistLeftLoc.Length();
}

FTransform ARide::GetCameraTransform() const
{
	return Camera->GetComponentTransform();
}

void ARide::RefreshRideCameraComponents()
{
	SpringArm->UpdateComponentToWorld();
	Camera->UpdateComponentToWorld();
}

FTransform ARide::GetSpringArmTransform() const
{
	return SpringArm->GetComponentTransform();;
}

float ARide::GetTargetArmLength() const
{
	return SpringArm->TargetArmLength;
}

FRotator ARide::GetControllerRotation() const
{
	return GetController()->GetControlRotation();
}

void ARide::PivotTurn(float TargetDeltaYaw)
{
	if (bPivotTurning)
		return;

	if (!PivotTurnMontage)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance || AnimInstance->Montage_IsPlaying(PivotTurnMontage))
		return;

	const float MontageLength = AnimInstance->Montage_Play(PivotTurnMontage);
	if (MontageLength <= 0.0f)
		return;

	PivotTurnTargetDeltaYaw = TargetDeltaYaw;
	PivotTurnDirection = PivotTurnTargetDeltaYaw >= 0.0f ? 1.0f : -1.0f;
	bPivotTurning = true;
	PivotTurnStartYaw = GetActorRotation().Yaw;
	PivotTurnPreviousYaw = PivotTurnStartYaw;
	CurrentThrottle = 0.0f;

	GetCharacterMovement()->StopMovementImmediately();

	AnimInstance->Montage_JumpToSection(PivotTurnDirection > 0.0f ? PivotTurnRightSection : PivotTurnLeftSection, PivotTurnMontage);
}

float ARide::GetRideSpeed() const
{ 

	return GetVelocity().Length();;
}

float ARide::GetRideDirection() const
{
	return GetDirection();
}

bool ARide::GetMountDir() const
{
	return MountRight;
}

FTransform ARide::GetMountTransform() const
{
	return RiderLocation->GetComponentTransform();
}

FTransform ARide::GetDismountTransform() const
{
	return RiderGetDownLoc ? RiderGetDownLoc->GetComponentTransform() : GetActorTransform();
}
