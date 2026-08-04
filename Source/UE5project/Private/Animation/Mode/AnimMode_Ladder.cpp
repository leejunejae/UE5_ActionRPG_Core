// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ladder.h"
#include "Characters/CharacterBase.h"
#include "Characters/CharacterBaseAnimInstance.h"
#include "Interaction/Climb/Components/ClimbComponent.h"

void UAnimMode_Ladder::Tick(float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (!Character.IsValid() || !AnimInst.IsValid()) return;

	auto* Ch = Character.Get();
	auto* Anim = AnimInst.Get();
	UClimbComponent* ClimbComponent = Ch->GetClimbComponent();
	if (!IsValid(ClimbComponent)) return;

	Anim->CurLadderStance = ClimbComponent->GetLadderStance();
	Anim->LadderActionState = ClimbComponent->GetLadderActionState();
	Anim->RepeatedStepExplicitTime = ClimbComponent->GetRepeatedStepExplicitTime();
	Anim->LadderIdleAnimation = ClimbComponent->GetLadderIdleAnimation();
	Anim->ActiveRepeatedStepAnimation = ClimbComponent->GetActiveRepeatedStepAnimation();

	static const FName HandLOffsetSocket(TEXT("Hand_L_Offset"));
	static const FName PalmLSocket(TEXT("Palm_L"));
	static const FName FootROffsetSocket(TEXT("Foot_R_Offset"));
	static const FName SoleRSocket(TEXT("Sole_R"));
	static const FName HandROffsetSocket(TEXT("Hand_R_Offset"));
	static const FName PalmRSocket(TEXT("Palm_R"));
	static const FName FootLOffsetSocket(TEXT("Foot_L_Offset"));
	static const FName SoleLSocket(TEXT("Sole_L"));

	const FVector Hand_L_Location = Character->GetMesh()->GetSocketLocation(HandLOffsetSocket);
	const FVector Palm_L_Location = Character->GetMesh()->GetSocketLocation(PalmLSocket);
	Anim->LeftHandLadderOffset = ClimbComponent->GetLimbIKTarget(ELimbList::HandL);
	Anim->LeftHandLadderOffset -= Palm_L_Location - Hand_L_Location;

	const FVector Foot_R_Location = Character->GetMesh()->GetSocketLocation(FootROffsetSocket);
	const FVector Sole_R_Location = Character->GetMesh()->GetSocketLocation(SoleRSocket);
	Anim->RightFootLadderOffset = ClimbComponent->GetLimbIKTarget(ELimbList::FootR);
	Anim->RightFootLadderOffset -= Sole_R_Location - Foot_R_Location;

	const FVector Hand_R_Location = Character->GetMesh()->GetSocketLocation(HandROffsetSocket);
	const FVector Palm_R_Location = Character->GetMesh()->GetSocketLocation(PalmRSocket);
	Anim->RightHandLadderOffset = ClimbComponent->GetLimbIKTarget(ELimbList::HandR);
	Anim->RightHandLadderOffset -= Palm_R_Location - Hand_R_Location;

	const FVector Foot_L_Location = Character->GetMesh()->GetSocketLocation(FootLOffsetSocket);
	const FVector Sole_L_Location = Character->GetMesh()->GetSocketLocation(SoleLSocket);
	Anim->LeftFootLadderOffset = ClimbComponent->GetLimbIKTarget(ELimbList::FootL);
	Anim->LeftFootLadderOffset -= Sole_L_Location - Foot_L_Location;
}
