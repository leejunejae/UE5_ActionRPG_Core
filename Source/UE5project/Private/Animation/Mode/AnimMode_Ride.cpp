// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ride.h"
#include "Characters/Rideable/Ride.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/PlayerBaseAnimInstance.h"

void UAnimMode_Ride::Tick(float DeltaSeconds)
{
	if (!Character.IsValid() || !AnimInst.IsValid()) return;

	auto* Ch = Cast<APlayerBase>(Character.Get());
	auto* Anim = AnimInst.Get();

	Anim->CurRideStance = Ch->GetCurRideStance();

	if (Anim->CurRideStance == ERideStance::Riding)
	{
		Anim->Speed = Ch->GetRideSpeed();
		Anim->Direction = Ch->GetRideDirection();
	}

	UpdateRideLocomotionIK(DeltaSeconds);
}

void UAnimMode_Ride::UpdateRideLocomotionIK(float DeltaSeconds)
{
	ARide* Ride = Character->GetCurrentRide();
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
