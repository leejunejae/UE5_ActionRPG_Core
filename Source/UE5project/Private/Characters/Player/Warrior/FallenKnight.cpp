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
	
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
}

void AFallenKnight::BeginPlay()
{
	Super::BeginPlay();

	GetEquipmentComponent()->EquipWeapon_Implementation(DefaultWeaponKey);

	GetEquipmentComponent()->EquipArmor(DefaultLegsKey);
	GetEquipmentComponent()->EquipArmor(DefaultHeadKey);
	GetEquipmentComponent()->EquipArmor(DefaultChestKey);
	GetEquipmentComponent()->EquipArmor(DefaultHandsKey);
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