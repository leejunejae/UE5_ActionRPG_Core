// Fill out your copyright notice in the Description page of Project Settings.

// 엔진 헤더
#include "Characters/Player/Warrior/FallenKnight.h"
#include "GameFramework/CharacterMovementComponent.h"

// 입력
#include "EnhancedInputComponent.h"

// Kismet 유틸리티
#include "Kismet/GameplayStatics.h"

// 애니메이션
#include "Characters/Player/Warrior/FallenKnightAnimInstance.h"

// 컴포넌트
#include "Characters/Components/EquipmentComponent.h"

AFallenKnight::AFallenKnight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MAIN_MESH(TEXT("/Game/04_Animations/Player/SK_DC_Knight_UE4_full_Silver.SK_DC_Knight_UE4_full_Silver"));
	if (MAIN_MESH.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MAIN_MESH.Object);
	}
	
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	static ConstructorHelpers::FClassFinder<UAnimInstance>FallenKnight_ANIM(TEXT("/Game/00_Character/Data/AnimData/Player_AnimBP.Player_AnimBP_C"));
	if (FallenKnight_ANIM.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(FallenKnight_ANIM.Class);
	}

	//GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	//GetCharacterMovement()->MaxAcceleration = 2048.0f;
	//GetCharacterMovement()->GroundFriction = 0.1f;
	//GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
}

void AFallenKnight::BeginPlay()
{
	Super::BeginPlay();

	GetEquipmentComponent()->EquipWeapon_Implementation(DefaultWeaponKey);
}

void AFallenKnight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFallenKnight::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{

	}
}

void AFallenKnight::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	//CharacterBaseAnim = Cast<UFallenKnightAnimInstance>(GetMesh()->GetAnimInstance());
}