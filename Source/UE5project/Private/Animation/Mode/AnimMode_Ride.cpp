// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ride.h"
#include "Characters/Components/RideComponent.h"
#include "Characters/Rideable/Ride.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/PlayerBaseAnimInstance.h"
#include "Characters/Rideable/RideProfileDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAnimMode_Ride::Tick(float DeltaSeconds)
{
	if (!Character.IsValid() || !AnimInst.IsValid()) return;

	auto* Ch = Cast<APlayerBase>(Character.Get());
	if (!Ch) return;

	auto* Anim = AnimInst.Get();

	Anim->IsInAir = Ch->GetCharacterMovement()->IsFalling();
	Anim->IsJumping = Anim->IsFalling = Anim->IsLanding = false;
	if (Anim->IsInAir)
	{
		Ch->GetVelocity().Z > 0.0f ? Anim->IsJumping = true : Anim->IsFalling = true;
	}

	FVector WorldAcceleration = Ch->GetCharacterMovement()->GetCurrentAcceleration() * FVector(1.0f, 1.0f, 0.0f);
	Anim->IsAccelerating = !WorldAcceleration.IsNearlyZero();

	if (URideComponent* RideComponent = Ch->GetRideComponent();
		RideComponent && RideComponent->GetRideActionState() == ERideActionState::Riding &&
		!RideComponent->IsRideTransitionAnimationActive())
	{
		const ARide* Ride = RideComponent->GetCurrentRide();
		const float SpeedInterpRate = Ride ? Ride->GetAnimationSpeedInterpRate() : 0.0f;
		const float TurnRateInterpRate = Ride ? Ride->GetAnimationTurnRateInterpRate() : 0.0f;
		Anim->Speed = FMath::FInterpTo(Anim->Speed, RideComponent->GetRideSpeed(), DeltaSeconds, SpeedInterpRate);
		Anim->Direction = RideComponent->GetRideDirection();

		if (Ride)
		{
			Anim->RideTurnRate = FMath::FInterpTo(
				Anim->RideTurnRate, Ride->GetTurnRate(), DeltaSeconds, TurnRateInterpRate);
			Anim->bRideBraking = Ride->IsBraking();
			Anim->RideGait = Ride->GetCurrentGait();
		}
	}

	UpdateRideLocomotionIK(DeltaSeconds);
}

void UAnimMode_Ride::UpdateRideLocomotionIK(float DeltaSeconds)
{
	APlayerBase* Player = Cast<APlayerBase>(Character.Get());
	if (!Player || !Player->GetRideComponent()) return;

	URideComponent* RideComponent = Player->GetRideComponent();
	ARide* Ride = RideComponent->GetCurrentRide();
	if (!Ride) return;

	USkeletalMeshComponent* RideMesh = Ride->GetMesh();
	const URideProfileDataAsset* Profile = RideComponent->GetRideProfile();
	if (!RideMesh || !Profile) return;

	FVector Hand_L_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_L_Offset"));
	FVector Palm_L_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_L"));
	AnimInst->IK_HandL_Ride_Locomotion = RideMesh->GetSocketLocation(Profile->HandLeftSocket);
	AnimInst->IK_HandL_Ride_Locomotion -= Palm_L_Location - Hand_L_Location;

	FVector Foot_R_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_R_Offset"));
	FVector Sole_R_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_R"));
	AnimInst->IK_FootR_Ride_Locomotion = RideMesh->GetSocketLocation(Profile->FootRightSocket);
	AnimInst->IK_FootR_Ride_Locomotion -= Sole_R_Location - Foot_R_Location;

	FVector Hand_R_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_R_Offset"));
	FVector Palm_R_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_R"));
	AnimInst->IK_HandR_Ride_Locomotion = RideMesh->GetSocketLocation(Profile->HandRightSocket);
	AnimInst->IK_HandR_Ride_Locomotion -= Palm_R_Location - Hand_R_Location;

	FVector Foot_L_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_L_Offset"));
	FVector Sole_L_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_L"));
	AnimInst->IK_FootL_Ride_Locomotion = RideMesh->GetSocketLocation(Profile->FootLeftSocket);
	AnimInst->IK_FootL_Ride_Locomotion -= Sole_L_Location - Foot_L_Location;
}
