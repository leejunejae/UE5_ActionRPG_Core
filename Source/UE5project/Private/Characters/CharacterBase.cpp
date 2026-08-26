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
#include "MotionWarpingComponent.h"
#include "Utils/CoreLog.h"
#include "Utils/GameplayTagsBase.h"

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

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

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

		GetCharacterStatusComponent()->OnRespawnStarted.AddUObject(this, &ACharacterBase::HandleRespawnStarted);
		GetCharacterStatusComponent()->OnRespawnFinalized.AddUObject(this, &ACharacterBase::HandleRespawnFinalized);
	}

}

void ACharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (HitReactionComponent && HitReactionComponent->IsHitAirReactionActive() &&
		!HitReactionComponent->TransitionHitAirToRecovery())
	{
		HitReactionComponent->CancelHitReaction(EActionExitReason::Interrupted, true);
	}

}

bool ACharacterBase::ApplyDirectHitStats(const FAttackRequest& AttackInfo, bool& bOutPoiseBroken)
{
	bOutPoiseBroken = false;
	if (!StatComponent)
	{
		return false;
	}

	const bool bSurvived = StatComponent->ApplyDamage(AttackInfo.Damage, AttackInfo.AttackType);
	if (!bSurvived)
	{
		return false;
	}

	const bool bWasPoiseBroken = StatComponent->GetCommonStats().GetPoise() <= 0.0f;
	StatComponent->ChangePoise(AttackInfo.PoiseDamage, EStatChangeType::Damage);
	const bool bIsPoiseBroken = StatComponent->GetCommonStats().GetPoise() <= 0.0f;
	bOutPoiseBroken = !bWasPoiseBroken && bIsPoiseBroken;
	return true;
}

void ACharacterBase::RestorePoise()
{
	if (StatComponent)
	{
		StatComponent->ChangePoise(
			StatComponent->GetCommonStats().GetMaxPoise(), EStatChangeType::Restore);
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

void ACharacterBase::HandleDeathStarted()
{
	if (HitReactionComponent)
	{
		HitReactionComponent->CancelHitReaction(EActionExitReason::Death, true);
	}
}

void ACharacterBase::HandleDeathFinalized()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 공용
}

void ACharacterBase::HandleRespawnStarted()
{
}

void ACharacterBase::HandleRespawnFinalized()
{
	// 사망 시 껐던 캡슐 콜리전 복구
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
