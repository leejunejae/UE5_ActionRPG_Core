// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ride.h"
#include "Characters/Components/RideComponent.h"
#include "Characters/Rideable/Ride.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/PlayerBaseAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAnimMode_Ride::Tick(float DeltaSeconds)
{
	if (!Character.IsValid() || !AnimInst.IsValid()) return;

	auto* Ch = Cast<APlayerBase>(Character.Get());
	if (!Ch) return;

	auto* Anim = AnimInst.Get();

	Anim->CurRideAnimPhase = Ch->GetCurRideAnimPhase();
	Anim->IsInAir = Ch->GetCharacterMovement()->IsFalling();
	Anim->IsJumping = Anim->IsFalling = Anim->IsLanding = false;
	if (Anim->IsInAir)
	{
		Ch->GetVelocity().Z > 0.0f ? Anim->IsJumping = true : Anim->IsFalling = true;
	}

	FVector WorldAcceleration = Ch->GetCharacterMovement()->GetCurrentAcceleration() * FVector(1.0f, 1.0f, 0.0f);
	Anim->IsAccelerating = !WorldAcceleration.IsNearlyZero();

	if (Anim->CurRideAnimPhase == ERideAnimPhase::Riding)
	{
		if (URideComponent* RideComponent = Ch->GetRideComponent())
		{
			Anim->Speed = RideComponent->GetRideSpeed();
			Anim->Direction = RideComponent->GetRideDirection();

			if (ARide* Ride = RideComponent->GetCurrentRide())
			{
				Anim->RideTurnRate = Ride->GetTurnRate();
				Anim->bRideBraking = Ride->IsBraking();
				Anim->RideGait = Ride->GetCurrentGait();
			}
		}
	}

	UpdateRideLocomotionIK(DeltaSeconds);
}

void UAnimMode_Ride::UpdateRideLocomotionIK(float DeltaSeconds)
{
	APlayerBase* Player = Cast<APlayerBase>(Character.Get());
	if (!Player || !Player->GetRideComponent()) return;

	ARide* Ride = Player->GetRideComponent()->GetCurrentRide();
	if (!Ride) return;

	USkeletalMeshComponent* RideMesh = Ride->GetMesh();
	if (!RideMesh) return;

	FVector Hand_L_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_L_Offset"));
	FVector Palm_L_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_L"));
	AnimInst->IK_HandL_Ride_Locomotion = RideMesh->GetSocketLocation(FName("Hand_L_RideIK"));
	AnimInst->IK_HandL_Ride_Locomotion -= Palm_L_Location - Hand_L_Location;

	FVector Foot_R_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_R_Offset"));
	FVector Sole_R_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_R"));
	AnimInst->IK_FootR_Ride_Locomotion = RideMesh->GetSocketLocation(FName("Foot_R_RideIK"));
	AnimInst->IK_FootR_Ride_Locomotion -= Sole_R_Location - Foot_R_Location;

	FVector Hand_R_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_R_Offset"));
	FVector Palm_R_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_R"));
	AnimInst->IK_HandR_Ride_Locomotion = RideMesh->GetSocketLocation(FName("Hand_R_RideIK"));
	AnimInst->IK_HandR_Ride_Locomotion -= Palm_R_Location - Hand_R_Location;

	FVector Foot_L_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_L_Offset"));
	FVector Sole_L_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_L"));
	AnimInst->IK_FootL_Ride_Locomotion = RideMesh->GetSocketLocation(FName("Foot_L_RideIK"));
	AnimInst->IK_FootL_Ride_Locomotion -= Sole_L_Location - Foot_L_Location;
}
