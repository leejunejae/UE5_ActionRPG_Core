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

	FVector Hand_L_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_L_Offset"));
	FVector Palm_L_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_L"));
	Anim->LeftHandLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::HandL);
	Anim->LeftHandLadderOffset -= Palm_L_Location - Hand_L_Location;

	FVector Foot_R_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_R_Offset"));
	FVector Sole_R_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_R"));
	Anim->RightFootLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::FootR);
	Anim->RightFootLadderOffset -= Sole_R_Location - Foot_R_Location;

	FVector Hand_R_Location = Character->GetMesh()->GetSocketLocation(FName("Hand_R_Offset"));
	FVector Palm_R_Location = Character->GetMesh()->GetSocketLocation(FName("Palm_R"));
	Anim->RightHandLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::HandR);
	Anim->RightHandLadderOffset -= Palm_R_Location - Hand_R_Location;

	FVector Foot_L_Location = Character->GetMesh()->GetSocketLocation(FName("Foot_L_Offset"));
	FVector Sole_L_Location = Character->GetMesh()->GetSocketLocation(FName("Sole_L"));
	Anim->LeftFootLadderOffset = Character->GetClimbComponent()->GetLimbIKTarget(ELimbList::FootL);
	Anim->LeftFootLadderOffset -= Sole_L_Location - Foot_L_Location;

	FIKContextWeights* Context = Anim->IKLayer.Find(
		FGameplayTag::RequestGameplayTag(TEXT("IK.Layer.Ladder.Climb")));

	if (Context)
	{
		float LH = Character->GetMesh()->GetSocketLocation(FName("hand_l")).Z;
		UE_LOG(Log_Anim_IK_Climb, Warning, TEXT("LH : %f  LH Target : %f"), LH, Anim->LeftHandLadderOffset.Z);
	}
}
