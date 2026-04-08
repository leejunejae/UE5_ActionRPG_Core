// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Components/BaseCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

float UBaseCharacterMovementComponent::GetGroundDistance(bool bForceUpdate)
{
    // 걷는 중/수영 등은 의미 없으니 리셋값 반환
    if (MovementMode != MOVE_Falling)
    {
        GroundDistance = BIG_NUMBER;
        LastUpdateFrame = GFrameCounter;
        LastUpdateTime = FPlatformTime::Seconds();
        return GroundDistance;
    }

    const uint64 CurFrame = GFrameCounter;
    const double Now = FPlatformTime::Seconds();

    const bool bFrameFresh = (LastUpdateFrame == CurFrame);
    const bool bTimeFresh = (SampleIntervalSec <= 0.0) || ((Now - LastUpdateTime) < SampleIntervalSec);

    if (!bForceUpdate && bFrameFresh && bTimeFresh)
    {
        return GroundDistance; // 이미 이번 프레임(혹은 주기 내) 계산함
    }

    UpdateGroundDistance();
    LastUpdateFrame = CurFrame;
    LastUpdateTime = Now;
    return GroundDistance;
}

void UBaseCharacterMovementComponent::UpdateGroundDistance()
{
    const ACharacter* Char = Cast<ACharacter>(GetOwner());
    if (!Char) { GroundDistance = BIG_NUMBER; return; }

    const UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
    check(Capsule);

    const float Radius = Capsule->GetScaledCapsuleRadius();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

    const FVector ActorLoc = Char->GetActorLocation();
    const FVector Start(ActorLoc);
    const FVector End(Start.X, Start.Y, (Start.Z - GroundTraceDistance - HalfHeight));

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GroundProbe), false, Char);
    FHitResult Hit;
    const ECollisionChannel Channel = ECC_Visibility; // 프로젝트에 맞게 커스텀 채널 추천

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, Start, End, Channel, Params
    );

    if (bHit)
    {
        const float BottomZ = ActorLoc.Z - HalfHeight;
        GroundDistance = FMath::Max(0.f, BottomZ - Hit.ImpactPoint.Z);
    }
    else
    {
        GroundDistance = BIG_NUMBER;
    }
}