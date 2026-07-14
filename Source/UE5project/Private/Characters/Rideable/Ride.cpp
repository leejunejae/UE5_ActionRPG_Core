// Fill out your copyright notice in the Description page of Project Settings.

// 기본 헤더
#include "Characters/Rideable/Ride.h"

// 카메라
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

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

// 인터페이스
#include "Characters/Player/Interfaces/PlayerInterface.h"

// 애니메이션
#include "Characters/Rideable/RideAnimInstance.h"




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
	
	SpringArm->TargetArmLength = 200.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f,0.0f,90.0f));
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
	SpringArm->SocketOffset = FVector(0.0f, 60.0f, 0.0f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bDoCollisionTest = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagSpeed = 10.0f;
	SpringArm->CameraRotationLagSpeed = 10.0f;

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->MaxWalkSpeed = 800.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 120.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
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
}

// Called when the game starts or when spawned
void ARide::BeginPlay()
{
	Super::BeginPlay();
	
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
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARide::Look);
		EnhancedInputComponent->BindAction(DisMountAction, ETriggerEvent::Triggered, this, &ARide::DisMount);
	}
}

void ARide::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

float ARide::GetDirection()
{
	return Direction;
}

void ARide::Move(const FInputActionValue& value)
{
	FVector2D DirectionValue = value.Get<FVector2D>();
	//MovementInputValue = value;
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	FVector2D MovementScale = DirectionValue;
	MovementScale.Normalize();

	//GetCharacterMovement()->GetLastInputVector();

	FVector MovementDirection = GetActorForwardVector();
	FVector LastInputDirection = (UKismetMathLibrary::GetForwardVector(YawRotation) * MovementScale.Y) + (UKismetMathLibrary::GetRightVector(YawRotation) * MovementScale.X);

	float DotProductDirection = FVector::DotProduct(MovementDirection, LastInputDirection);
	float DotProductRadian = FMath::Acos(DotProductDirection);
	float DotProductDegree = FMath::RadiansToDegrees(DotProductRadian);

	FVector RotationAxis = FVector::CrossProduct(MovementDirection, LastInputDirection);
	RotationAxis.Normalize();

	Direction = RotationAxis.Z > 0.0f ? DotProductDegree : -1.0f * DotProductDegree;

	if (DotProductDegree > 160.0f)
	{
		QuickTurn(RotationAxis.Z);
		return;
	}

	float AngleRadians = DotProductDegree > 5.0f ? FMath::DegreesToRadians(5.0f) : FMath::DegreesToRadians(DotProductDegree);

	FQuat RotationQuat = FQuat(RotationAxis, AngleRadians);
	FVector RotatedVector = RotationQuat.RotateVector(MovementDirection);
	
	AddMovementInput(RotatedVector);

	if (!LastInputDirection.IsNearlyZero())
	{
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

void ARide::Look(const FInputActionValue& value)
{
	const FVector2D LookAxisValue = value.Get<FVector2D>();
	AddControllerPitchInput(LookAxisValue.Y * 0.5f);
	AddControllerYawInput(LookAxisValue.X * -0.5f);
}

void ARide::Mount_Implementation(ACharacter* RiderCharacter, FVector InitVelocity)
{
	IRideInterface::Mount_Implementation(RiderCharacter, InitVelocity);

	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;

	FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld,
		EAttachmentRule::KeepRelative,
		true
	);

	Rider = RiderCharacter;

	Rider->AttachToComponent(RiderLocation, AttachmentRules);
	//Rider->AttachToComponent(GetMesh(), AttachmentRules, FName("MountPoint"));

	FTransform SpringArmTransform = IViewDataInterface::Execute_GetSpringArmTransform(Rider);
	SpringArm->SetWorldLocation(SpringArmTransform.GetLocation());
	SpringArm->SetWorldRotation(SpringArmTransform.GetRotation().Rotator());

	FRotator InitControllerRotator = IViewDataInterface::Execute_GetControllerRotation(Rider);

	UGameplayStatics::GetPlayerController(GetWorld(), 0)->Possess(this);
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetControlRotation(InitControllerRotator);

	GetCharacterMovement()->Velocity = InitVelocity;

	CanDismount = false;

	GetWorldTimerManager().SetTimer(CameraSettingTimerHandle, this, &ARide::CameraSettingTimer, 0.01f, true);
}

void ARide::DisMount()
{
	TryDisMount();
}

bool ARide::TryDisMount()
{
	if (!Rider || !RiderGetDownLoc || !CanDismount)
		return false;

	if (Rider->GetClass()->ImplementsInterface(UPlayerInterface::StaticClass()))
	{
		LastSpeed = GetVelocity();

		IPlayerInterface::Execute_DespawnRide(Rider, GetVelocity());

		bDismount = true;
		Rider = nullptr;
	}

	return true;
}

bool ARide::FindMountPos()
{
	FVector DistRightLoc = Rider->GetActorLocation() - RiderMountLocRight->GetComponentLocation();
	FVector DistLeftLoc = Rider->GetActorLocation() - RiderMountLocLeft->GetComponentLocation();

	return DistRightLoc.Length() < DistLeftLoc.Length();
}

void ARide::CameraSettingTimer()
{
	float CurrentLength = SpringArm->TargetArmLength;
	float NewLength = FMath::FInterpTo(CurrentLength, 300.0f, GetWorld()->GetDeltaSeconds(), 1.0f);

	SpringArm->TargetArmLength = NewLength;

	if (FMath::IsNearlyEqual(NewLength, 300.0f, 1.0f))
	{
		SpringArm->TargetArmLength = 300.0f;
		GetWorld()->GetTimerManager().ClearTimer(CameraSettingTimerHandle);
		SpringArm->bEnableCameraLag = true;
		SpringArm->bEnableCameraRotationLag = true;
	}
}

FTransform ARide::GetCameraTransform_Implementation()
{
	return Camera->GetComponentTransform();
}

FTransform ARide::GetSpringArmTransform_Implementation()
{
	return SpringArm->GetComponentTransform();;
}

float ARide::GetTargetArmLength_Implementation()
{
	return SpringArm->TargetArmLength;
}

FRotator ARide::GetControllerRotation_Implementation()
{
	return GetController()->GetControlRotation();
}

void ARide::QuickTurn(float TurnDirection)
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		// 이미 재생 중이면 무시
		if (AnimInstance->Montage_IsPlaying(TurnMontage))
			return;

		AnimInstance->Montage_Play(TurnMontage);
		AnimInstance->Montage_JumpToSection(FName("TurnLeft"));
	}
}

float ARide::GetRideSpeed_Implementation()
{ 

	return GetVelocity().Length();;
}

float ARide::GetRideDirection_Implementation()
{
	return GetDirection();
}

bool ARide::GetMountDir_Implementation()
{
	return MountRight;
}

FTransform ARide::GetMountTransform_Implementation()
{
	return RiderLocation->GetComponentTransform();
}
