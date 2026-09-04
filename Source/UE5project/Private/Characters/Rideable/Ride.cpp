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
#include "Characters/Rideable/RideProfileDataAsset.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/InputConfigDataAsset.h"

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

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	
	static ConstructorHelpers::FClassFinder<UAnimInstance>HORSE_ANIM(TEXT("/Game/05_Ride/AnimData/RHAB_AnimBlueprint.RHAB_AnimBlueprint_C"));
	if (HORSE_ANIM.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(HORSE_ANIM.Class);
	}

	Tags.Add("Ride");
}

// Called when the game starts or when spawned
void ARide::BeginPlay()
{
	Super::BeginPlay();

	MaxRideSpeed = FMath::Max(WalkRideSpeed, FMath::Max(RunRideSpeed, SprintRideSpeed));
	GetCharacterMovement()->MaxWalkSpeed = MaxRideSpeed;
	
}

void ARide::ActivateRideInputContext()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !InputConfig) return;
	if (UEnhancedInputLocalPlayerSubsystem* SubSystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		if (InputConfig->RideContext)
		{
			if (InputConfig->DefaultContext) SubSystem->RemoveMappingContext(InputConfig->DefaultContext);
			SubSystem->AddMappingContext(InputConfig->RideContext, 0);
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
		UpdateDodgeSprintInput(DeltaTime);
		UpdateRideMovement(DeltaTime);
	}


	FRotator SocketRot = GetMesh()->GetSocketRotation(FName("MountPoint"));
	FRotator CurrentRot = RiderLocation->GetComponentRotation();
	const FRotator SmoothedRot = FMath::RInterpTo(CurrentRot, SocketRot, DeltaTime, 5.0f);
	RiderLocation->SetWorldRotation(SmoothedRot);
	RiderLocation->SetWorldLocation(GetMesh()->GetSocketLocation(FName("MountPoint")));
}

// Called to bind functionality to input
void ARide::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (InputConfig && InputConfig->Move)
		{
			EnhancedInputComponent->BindAction(InputConfig->Move, ETriggerEvent::Triggered, this, &ARide::Move);
			EnhancedInputComponent->BindAction(InputConfig->Move, ETriggerEvent::Completed, this, &ARide::StopMove);
			EnhancedInputComponent->BindAction(InputConfig->Move, ETriggerEvent::Canceled, this, &ARide::StopMove);
		}
		if (InputConfig && InputConfig->Look)
		{
			EnhancedInputComponent->BindAction(InputConfig->Look, ETriggerEvent::Triggered, this, &ARide::Look);
		}
		UInputAction* DismountAction = InputConfig
			? (InputConfig->Dismount ? InputConfig->Dismount.Get() : InputConfig->SpawnRide.Get()) : nullptr;
		if (DismountAction)
		{
			EnhancedInputComponent->BindAction(DismountAction, ETriggerEvent::Started, this, &ARide::DisMount);
			EnhancedInputComponent->BindAction(DismountAction, ETriggerEvent::Completed, this, &ARide::DisMountInputCompleted);
		}

		if (InputConfig && InputConfig->Walk)
		{
			EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Started, this, &ARide::StartWalk);
			EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Completed, this, &ARide::StopWalk);
			EnhancedInputComponent->BindAction(InputConfig->Walk, ETriggerEvent::Canceled, this, &ARide::StopWalk);
		}

		if (InputConfig && InputConfig->Dodge)
		{
			EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Started, this, &ARide::DodgeSprintInputStarted);
			EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Completed, this, &ARide::DodgeSprintInputCompleted);
			EnhancedInputComponent->BindAction(InputConfig->Dodge, ETriggerEvent::Canceled, this, &ARide::DodgeSprintInputCanceled);
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

void ARide::DodgeSprintInputStarted()
{
	bDodgeSprintInputHeld = true;
	DodgeSprintHeldTime = 0.0f;
}

void ARide::DodgeSprintInputCompleted()
{
	bDodgeSprintInputHeld = false;
	DodgeSprintHeldTime = 0.0f;
	bWantsSprint = false;
}

void ARide::DodgeSprintInputCanceled()
{
	bDodgeSprintInputHeld = false;
	DodgeSprintHeldTime = 0.0f;
	bWantsSprint = false;
}

void ARide::UpdateDodgeSprintInput(float DeltaTime)
{
	if (!bDodgeSprintInputHeld || bWantsSprint) return;
	DodgeSprintHeldTime += FMath::Max(0.0f, DeltaTime);
	if (DodgeSprintHeldTime >= SprintHoldThreshold)
	{
		bWantsSprint = true;
	}
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

	const float InputMagnitude = FMath::Clamp(RawInput.Size(), 0.0f, 1.0f);
	const bool bHasMoveInput = InputMagnitude > InputDeadZone;

	float TargetThrottle = 0.0f;
	float TargetDirection = 0.0f;
	float DesiredTurnRate = 0.0f;

	if (bHasMoveInput)
	{
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		FVector2D MovementScale = RawInput;
		MovementScale.Normalize();

		FVector LastInputDirection = (UKismetMathLibrary::GetForwardVector(YawRotation) * MovementScale.Y) + (UKismetMathLibrary::GetRightVector(YawRotation) * MovementScale.X);
		LastInputDirection.Z = 0.0f;
		LastInputDirection.Normalize();

		const float DesiredYaw = LastInputDirection.Rotation().Yaw;
		const float SignedDirection = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredYaw);
		const float DirectionMagnitude = FMath::Abs(SignedDirection);
		const float ForwardAlignment = FVector::DotProduct(GetActorForwardVector(), LastInputDirection);
		const ERideGait DesiredGait = GetDesiredRideGait(RawInput, ForwardAlignment);
		const float TargetSpeed = GetRideSpeedForGait(DesiredGait);
		TargetThrottle = FMath::Clamp(TargetSpeed / FMath::Max(MaxRideSpeed, 1.0f), 0.0f, 1.0f);

		// Convert the full steering angle into the much narrower blend-space
		// domain. Feeding a -90..90 value into a -20..20 blend space causes even
		// small steering changes to snap immediately to an edge sample.
		TargetDirection = FMath::Clamp(
			SignedDirection / FMath::Max(MaxAnimDirection, 1.0f), -1.0f, 1.0f) * BlendSpaceDirectionLimit;

		if (DirectionMagnitude > PivotTurnMinAngle)
		{
			if (CanStartPivotTurn(DirectionMagnitude))
			{
				const float TargetDeltaYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredYaw);
				PivotTurn(TargetDeltaYaw);
				return;
			}

			TargetThrottle = 0.0f;
		}

		const float SpeedAlpha = FMath::Clamp(GetVelocity().Size2D() / FMath::Max(MaxRideSpeed, 1.0f), 0.0f, 1.0f);
		const float TurnRateBySpeed = FMath::Lerp(MaxTurnRate, MinTurnRateAtMaxSpeed, SpeedAlpha);
		// Steering uses the unscaled world-space heading error. TargetDirection is
		// intentionally compressed to the blend-space range and must not affect
		// the horse's physical turning authority.
		const float Steering = FMath::Clamp(
			SignedDirection / FMath::Max(MaxAnimDirection, 1.0f), -1.0f, 1.0f);
		DesiredTurnRate = Steering * TurnRateBySpeed;

		// Preserve analog stick magnitude instead of converting every non-zero
		// input into the full speed of the selected gait.
		TargetThrottle *= InputMagnitude;
	}

	const float InterpSpeed = TargetThrottle > CurrentThrottle ? AccelerationInterpSpeed : DecelerationInterpSpeed;
	CurrentThrottle = FMath::FInterpTo(CurrentThrottle, TargetThrottle, DeltaTime, InterpSpeed);
	Direction = FMath::FInterpTo(Direction, TargetDirection, DeltaTime, AnimationDirectionInterpRate);
	const float TurnAuthority = bHasMoveInput
		? FMath::Lerp(
			MinMovingTurnAuthority,
			1.0f,
			FMath::Clamp(GetVelocity().Size2D() / FMath::Max(FullTurnAuthoritySpeed, 1.0f), 0.0f, 1.0f))
		: 0.0f;
	DesiredTurnRate *= TurnAuthority;
	TurnRate = FMath::FInterpTo(TurnRate, DesiredTurnRate, DeltaTime, TurnRateInterpSpeed);
	AddActorWorldRotation(FRotator(0.0f, TurnRate * DeltaTime, 0.0f));

	// CharacterMovement keeps its previous planar velocity while the actor turns.
	// Gradually redirect that velocity toward the horse's new heading to avoid a
	// visible sideways slide without removing all of the horse's momentum.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		const FVector CurrentVelocity = Movement->Velocity;
		const float PlanarSpeed = CurrentVelocity.Size2D();
		if (PlanarSpeed > KINDA_SMALL_NUMBER)
		{
			const FVector DesiredPlanarVelocity = GetActorForwardVector() * PlanarSpeed;
			FVector RedirectedVelocity = FMath::VInterpTo(
				FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f),
				FVector(DesiredPlanarVelocity.X, DesiredPlanarVelocity.Y, 0.0f),
				DeltaTime,
				VelocityHeadingInterpSpeed);
			RedirectedVelocity.Z = CurrentVelocity.Z;
			Movement->Velocity = RedirectedVelocity;
		}
	}

	if (CurrentThrottle > KINDA_SMALL_NUMBER)
	{
		AddMovementInput(GetActorForwardVector(), CurrentThrottle);
	}

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

ERideGait ARide::GetDesiredRideGait(const FVector2D& MoveInput, float ForwardAlignment) const
{
	if (MoveInput.SizeSquared() <= FMath::Square(InputDeadZone))
		return ERideGait::Idle;

	if (bWantsWalk || MoveInput.Size() < WalkInputThreshold)
		return ERideGait::Walk;

	// Sprint is based on the desired world-space travel direction, not the raw
	// input Y axis. A camera rotated 90 degrees can make right input point exactly
	// along the horse's forward direction.
	if (bWantsSprint && ForwardAlignment >= SprintForwardAlignmentThreshold)
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
	if (const APlayerBase* Player = Cast<APlayerBase>(RiderCharacter))
	{
		InputConfig = Player->GetInputConfig();
	}
	GetCharacterMovement()->Velocity = InitVelocity;

	bDismount = false;
	bMovingDismount = false;
	bDodgeSprintInputHeld = false;
	bWantsSprint = false;
	DodgeSprintHeldTime = 0.0f;
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
	if (Rider)
	{
		if (URideComponent* RideComponent = Rider->FindComponentByClass<URideComponent>())
		{
			RideComponent->HandleRideInputStarted();
		}
	}
}

void ARide::ApplyRideProfile(const URideProfileDataAsset* Profile)
{
	if (!IsValid(Profile))
	{
		return;
	}

	WalkRideSpeed = Profile->WalkSpeed;
	RunRideSpeed = Profile->RunSpeed;
	SprintRideSpeed = Profile->SprintSpeed;
	MaxRideSpeed = FMath::Max(WalkRideSpeed, FMath::Max(RunRideSpeed, SprintRideSpeed));
	WalkInputThreshold = Profile->WalkInputThreshold;
	AccelerationInterpSpeed = Profile->AccelerationInterpSpeed;
	DecelerationInterpSpeed = Profile->DecelerationInterpSpeed;
	MaxTurnRate = Profile->MaxTurnRate;
	MinTurnRateAtMaxSpeed = Profile->MinTurnRateAtMaxSpeed;
	TurnRateInterpSpeed = Profile->TurnRateInterpSpeed;
	VelocityHeadingInterpSpeed = Profile->VelocityHeadingInterpSpeed;
	FullTurnAuthoritySpeed = Profile->FullTurnAuthoritySpeed;
	MinMovingTurnAuthority = Profile->MinMovingTurnAuthority;
	PivotTurnMinAngle = Profile->PivotTurnMinAngle;
	InputDeadZone = Profile->InputDeadZone;
	MaxAnimDirection = Profile->MaxAnimDirection;
	BlendSpaceDirectionLimit = Profile->BlendSpaceDirectionLimit;
	AnimationDirectionInterpRate = Profile->AnimationDirectionInterpRate;
	SprintForwardAlignmentThreshold = Profile->SprintForwardAlignmentThreshold;
	AnimationSpeedInterpRate = Profile->AnimationSpeedInterpRate;
	AnimationTurnRateInterpRate = Profile->AnimationTurnRateInterpRate;
	PivotTurnMaxStartSpeed = Profile->PivotTurnMaxStartSpeed;
	PivotTurnMontage = Profile->PivotTurnMontage;
	PivotTurnAlphaCurve = Profile->PivotTurnAlphaCurve;
	bUseNormalizedPivotTurnCurveTime = Profile->bUseNormalizedPivotTurnCurveTime;
	PivotTurnLeftSection = Profile->PivotTurnLeftSection;
	PivotTurnRightSection = Profile->PivotTurnRightSection;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = MaxRideSpeed;
	}
}

void ARide::DisMountInputCompleted()
{
	if (Rider)
	{
		if (URideComponent* RideComponent = Rider->FindComponentByClass<URideComponent>())
		{
			RideComponent->HandleRideInputCompleted();
		}
	}
}

void ARide::NotifyDismountStarted(bool bMoving)
{
	bMovingDismount = bMoving;
	bDismount = bMoving;
	Rider = nullptr;
	RideMoveInput = FVector2D::ZeroVector;
	bWantsWalk = false;
	bWantsSprint = false;
}

void ARide::ReleaseRider(bool bContinueForward)
{
	Rider = nullptr;
	bDismount = bContinueForward;
	bMovingDismount = bContinueForward;
	RideMoveInput = FVector2D::ZeroVector;
	if (!bContinueForward)
	{
		CurrentThrottle = 0.0f;
	}
	bWantsWalk = false;
	bWantsSprint = false;
	if (bPivotTurning)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (PivotTurnMontage && AnimInstance->Montage_IsPlaying(PivotTurnMontage))
			{
				AnimInstance->Montage_Stop(0.1f, PivotTurnMontage);
			}
		}
		bPivotTurning = false;
		PivotTurnDirection = 0.0f;
		PivotTurnTargetDeltaYaw = 0.0f;
		Direction = 0.0f;
		TurnRate = 0.0f;
	}
}

void ARide::FinishDismount()
{
	Destroy();
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
	return GetController() ? GetController()->GetControlRotation() : GetActorRotation();
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

FTransform ARide::GetMountTransform() const
{
	return RiderLocation ? RiderLocation->GetComponentTransform() : GetActorTransform();
}

FTransform ARide::GetDismountTransform() const
{
	return RiderGetDownLoc ? RiderGetDownLoc->GetComponentTransform() : GetActorTransform();
}
