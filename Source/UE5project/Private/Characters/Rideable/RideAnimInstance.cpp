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

		Speed = Character->GetVelocity().Length();//FMath::FInterpTo(Speed, VelocityLength, DeltaSeconds, 20.0f);
		Direction = Character->GetDirection();

		//UE_LOG(LogTemp, Warning, TEXT("Speed = %f"), Speed);

		if (GetCurveValue(FName("TurnLock")) > 0.0f)
		{
			bQuickTurn = false;
		}
		else
		{
			bQuickTurn = Direction > 160.0f || Direction < -160.0f ? bQuickTurn = true : bQuickTurn = false;
		}
	}
}
