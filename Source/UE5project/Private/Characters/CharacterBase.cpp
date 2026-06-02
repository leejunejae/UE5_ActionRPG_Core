// Fill out your copyright notice in the Description page of Project Settings.

// 기본 헤더
#include "Characters/CharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/Components/AttackComponent.h"
#include "Combat/Components/HitReactionComponent.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "Characters/Components/StatComponent.h"
#include "Interaction/Climb/Components/ClimbComponent.h"
#include "Characters/Rideable/Ride.h"
#include "Utils/CoreLog.h"

// Sets default values
ACharacterBase::ACharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->bAutoActivate = true;

	HitReactionComponent = CreateDefaultSubobject<UHitReactionComponent>(TEXT("HitReactionComponent"));
	HitReactionComponent->bAutoActivate = true;

	CharacterStatusComponent = CreateDefaultSubobject<UCharacterStatusComponent>(TEXT("CharacterStatusComponent"));
	CharacterStatusComponent->bAutoActivate = true;

	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	StatComponent->bAutoActivate = true;

	ClimbComponent = CreateDefaultSubobject<UClimbComponent>(TEXT("ClimbComponent"));
	ClimbComponent->bAutoActivate = true;

	ClimbComponent->SetMinGripInterval(MinGripInterval);
	ClimbComponent->SetMaxGripInterval(MaxGripInterval);
	ClimbComponent->SetMinFirstGripHeight(MinFirstGripHeight);


	GetCharacterMovement()->bEnablePhysicsInteraction = false;

	TeamID = 0;
}

// Called when the game starts or when spawned
void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (GetCharacterStatusComponent())
	{
		GetCharacterStatusComponent()->OnDeathStarted.AddUObject(this, &ACharacterBase::HandleDeathStarted);
		GetCharacterStatusComponent()->OnDeathFinalized.AddUObject(this, &ACharacterBase::HandleDeathFinalized);
	}

	if (GetHitReactionComponent())
	{
		GetHitReactionComponent()->HitEndDelegate.AddUObject(GetCharacterStatusComponent(), &UCharacterStatusComponent::ClearAction);
	}
}

void ACharacterBase::SetCurLocomotionGait(ELocomotionGait NewGait)
{
	if (!GaitData.Find(NewGait)->bEnabled)
	{
		UE_LOG(Log_Check, Warning, TEXT("[CharacterBase] '%s' not found in GaitList"), *StaticEnum<ELocomotionGait>()->GetValueAsString(NewGait));
		return;
	}

	UE_LOG(Log_Check, Log, TEXT("[CharacterBase] Character : %s LocomotionGait is Changed"), *GetName());

	CurLocomotionGait = NewGait;

	FGaitSetting NewGaitSetting = *GaitData.Find(CurLocomotionGait);

	GetCharacterMovement()->MaxWalkSpeed = NewGaitSetting.MaxSpeed;
}

ARide* ACharacterBase::GetCurrentRide()
{
	return Ride;
}


void ACharacterBase::HandleDeathStarted()
{

}

void ACharacterBase::HandleDeathFinalized()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 공용
}