// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ladder.h"
#include "Utils/CoreLog.h"
#include "Characters/CharacterBase.h"
#include "Characters/CharacterBaseAnimInstance.h"
#include "Interaction/Climb/Components/ClimbComponent.h"

void UAnimMode_Ladder::Tick(float DeltaSeconds)
{
	if (!Character.IsValid() || !AnimInst.IsValid()) return;

	auto* Ch = Character.Get();
	auto* Anim = AnimInst.Get();

	Anim->CurLadderStance = Ch->GetClimbComponent()->GetLadderStance_Native();

	Anim->LeftHandLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::HandL);
	FVector Hand_L_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_L_Offset"));
	FVector Palm_L_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_L"));
	Anim->LeftHandLadderOffset -= Palm_L_Location - Hand_L_Location;

	Anim->RightFootLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::FootR);
	FVector Foot_R_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_R_Offset"));
	FVector Sole_R_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_R"));
	Anim->RightFootLadderOffset -= Sole_R_Location - Foot_R_Location;

	Anim->RightHandLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::HandR);
	FVector Hand_R_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_R_Offset"));
	FVector Palm_R_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_R"));
	Anim->RightHandLadderOffset -= Palm_R_Location - Hand_R_Location;

	Anim->LeftFootLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::FootL);
	FVector Foot_L_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_L_Offset"));
	FVector Sole_L_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_L"));
	Anim->LeftFootLadderOffset -= Sole_L_Location - Foot_L_Location;

	//UE_LOG(Log_Anim_IK_Climb, Log, TEXT("[AnimMode_Ladder] LeftHand Target = X : %f, Y : %f, Z : %f"), Anim->LeftHandLadderOffset.X, Anim->LeftHandLadderOffset.Y, Anim->LeftHandLadderOffset.Z);
	//UE_LOG(Log_Anim_IK_Climb, Log, TEXT("[AnimMode_Ladder] RightHand Target = X : %f, Y : %f, Z : %f"), Anim->RightHandLadderOffset.X, Anim->RightHandLadderOffset.Y, Anim->RightHandLadderOffset.Z);
	//UE_LOG(Log_Anim_IK_Climb, Log, TEXT("[AnimMode_Ladder] LeftFoot Target = X : %f, Y : %f, Z : %f"), Anim->LeftFootLadderOffset.X, Anim->LeftFootLadderOffset.Y, Anim->LeftFootLadderOffset.Z);
	//UE_LOG(Log_Anim_IK_Climb, Log, TEXT("[AnimMode_Ladder] RightFoot Target = X : %f, Y : %f, Z : %f"), Anim->RightFootLadderOffset.X, Anim->RightFootLadderOffset.Y, Anim->RightFootLadderOffset.Z);
}
