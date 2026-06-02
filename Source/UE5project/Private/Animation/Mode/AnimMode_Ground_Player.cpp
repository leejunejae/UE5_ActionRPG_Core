// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ground_Player.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/PlayerBaseAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/Player/Components/LockOnComponent.h"
#include "Characters/Player/Components/PlayerStatusComponent.h"
#include "Utils/GameplayTagsBase.h"
#include "Kismet/KismetMathLibrary.h"

void UAnimMode_Ground_Player::OnModeEnter()
{
	Super::OnModeEnter();

	if (Character.Get())
		PrevYaw = Character->GetActorRotation().Yaw;
}

void UAnimMode_Ground_Player::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Character.IsValid() || !AnimInst.IsValid()) return;
	auto* Ch = Cast<APlayerBase>(Character.Get());
	auto* Anim = Cast<UPlayerBaseAnimInstance>(AnimInst.Get());

	AnimInst->Speed = Character->GetVelocity().Length();

	// 가속 확인
	FVector WorldAcceleration = Character->GetCharacterMovement()->GetCurrentAcceleration() * FVector(1.0f, 1.0f, 0.0f);
	FVector LocalAcceleration = Character->GetActorRotation().UnrotateVector(WorldAcceleration);
	float AccelerationLength = LocalAcceleration.SizeSquared();
	AnimInst->IsAccelerating = !FMath::IsNearlyZero(AccelerationLength);

	Anim->LeanAngle = ComputeLeanYawPerSecond(DeltaSeconds);                    // ← GetAnimDirection 대체

	if (Anim->Speed > 100.0f)
	{
		Anim->LeftFootGroundOffset.X = FMath::FInterpTo(Anim->LeftFootGroundOffset.X, 5.0f, DeltaSeconds, 5.0f);
		Anim->RightFootGroundOffset.X = FMath::FInterpTo(Anim->RightFootGroundOffset.X, -5.0f, DeltaSeconds, 5.0f);
	}
	else
	{
		Anim->LeftFootGroundOffset.X = FMath::FInterpTo(Anim->LeftFootGroundOffset.X, 0.0f, DeltaSeconds, 5.0f);
		Anim->RightFootGroundOffset.X = FMath::FInterpTo(Anim->RightFootGroundOffset.X, 0.0f, DeltaSeconds, 5.0f);
	}

	AnimInst->bIsLockOn = Ch->GetLockOnComponent()->IsLockedOn();

	float Target = Ch->GetCharacterStatusComponent()->GetCurrentAction() == TAG_Action_Guard ? 1.f : 0.f;
	AnimInst->GuardBlend = FMath::FInterpTo(AnimInst->GuardBlend, Target, DeltaSeconds, 5.f);
}

float UAnimMode_Ground_Player::ComputeLeanYawPerSecond(float DeltaSeconds)
{
	if (DeltaSeconds <= KINDA_SMALL_NUMBER) return 0.f;
	const float CurYaw = Character->GetActorRotation().Yaw;
	const float YawDelta = CurYaw - PrevYaw;
	PrevYaw = CurYaw;
	const float YawPerSec = UKismetMathLibrary::SafeDivide(YawDelta, DeltaSeconds);
	return YawPerSec * LeanScale; // = Direction:contentReference[oaicite:4]{index=4}
}