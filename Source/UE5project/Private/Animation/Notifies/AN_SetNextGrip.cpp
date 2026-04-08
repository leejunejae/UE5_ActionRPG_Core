// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_SetNextGrip.h"
#include "Interaction/Climb/Interfaces/ClimbInterface.h"

void UAN_SetNextGrip::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	/*
	if (MeshComp)
	{
		if (MeshComp->GetOwner()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
		{
			IClimbInterface::Execute_SetNextGripDown(MeshComp->GetOwner(), TargetBone, Count);
		}
	}
	*/
}