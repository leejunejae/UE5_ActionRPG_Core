// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_SetRotation.h"
#include "Characters/Interfaces/CharacterTransformInterface.h"

void UAN_SetRotation::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner() || !Animation)
	{
		return;
	}

	if (MeshComp->GetOwner()->GetClass()->ImplementsInterface(UCharacterTransformInterface::StaticClass()))
	{
		ICharacterTransformInterface::Execute_SetRotationInputDirection(MeshComp->GetOwner());
	}
}