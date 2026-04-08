// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ground_NPC.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/EnemyBaseAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAnimMode_Ground_NPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Character.IsValid() || !AnimInst.IsValid()) return;
	auto* Ch = Cast<AEnemyBase>(Character.Get());
	auto* Anim = Cast<UEnemyBaseAnimInstance>(AnimInst.Get());

	// 가속 확인
	FVector WorldAcceleration = Character->GetCharacterMovement()->GetCurrentAcceleration() * FVector(1.0f, 1.0f, 0.0f);
	FVector LocalAcceleration = Character->GetActorRotation().UnrotateVector(WorldAcceleration);
	float AccelerationLength = LocalAcceleration.SizeSquared();
	AnimInst->IsAccelerating = Character->GetVelocity().SizeSquared2D() > 10.0f;
		//!FMath::IsNearlyZero(AccelerationLength);
}
