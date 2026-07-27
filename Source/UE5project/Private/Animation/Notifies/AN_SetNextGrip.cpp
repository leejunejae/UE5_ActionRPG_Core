// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_SetNextGrip.h"

void UAN_SetNextGrip::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
}
