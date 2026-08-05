// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Mode/AnimMode_Ladder.h"
#include "Characters/CharacterBase.h"
#include "Characters/CharacterBaseAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interaction/Climb/Components/ClimbComponent.h"

void UAnimMode_Ladder::Tick(float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (!Character.IsValid() || !AnimInst.IsValid()) return;

	auto* Ch = Character.Get();
	auto* Anim = AnimInst.Get();
	UClimbComponent* ClimbComponent = Ch->GetClimbComponent();
	if (!IsValid(ClimbComponent)) return;
	USkeletalMeshComponent* Mesh = Ch->GetMesh();
	if (!IsValid(Mesh)) return;

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

	const auto UpdateLimbTarget =
		[ClimbComponent, Mesh](
			ELimbList Limb,
			FName OffsetSocket,
			FName ContactSocket,
			FVector& OutTarget)
		{
			OutTarget = ClimbComponent->GetLimbIKTarget(Limb);
			if (Mesh->DoesSocketExist(OffsetSocket) &&
				Mesh->DoesSocketExist(ContactSocket))
			{
				OutTarget -= Mesh->GetSocketLocation(ContactSocket) -
					Mesh->GetSocketLocation(OffsetSocket);
			}
		};

	UpdateLimbTarget(ELimbList::HandL, HandLOffsetSocket, PalmLSocket, Anim->LeftHandLadderOffset);
	UpdateLimbTarget(ELimbList::FootR, FootROffsetSocket, SoleRSocket, Anim->RightFootLadderOffset);
	UpdateLimbTarget(ELimbList::HandR, HandROffsetSocket, PalmRSocket, Anim->RightHandLadderOffset);
	UpdateLimbTarget(ELimbList::FootL, FootLOffsetSocket, SoleLSocket, Anim->LeftFootLadderOffset);
}
