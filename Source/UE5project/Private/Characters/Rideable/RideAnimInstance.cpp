// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Rideable/RideAnimInstance.h"
#include "Characters/Rideable/Ride.h"

URideAnimInstance::URideAnimInstance()
{

}

void URideAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Character = Cast<ARide>(TryGetPawnOwner());
}

void URideAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (Character)
	{

		Speed = FMath::FInterpTo(
			Speed,
			Character->GetVelocity().Size2D(),
			DeltaSeconds,
			Character->GetAnimationSpeedInterpRate());
		Direction = Character->GetDirection();
		TurnRate = FMath::FInterpTo(
			TurnRate,
			Character->GetTurnRate(),
			DeltaSeconds,
			Character->GetAnimationTurnRateInterpRate());
		bBraking = Character->IsBraking();
		CurrentGait = Character->GetCurrentGait();
	}
}
