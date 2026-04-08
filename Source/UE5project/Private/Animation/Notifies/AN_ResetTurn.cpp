// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_ResetTurn.h"
#include "Animation/Interfaces/IAnimInstance.h"

void UAN_ResetTurn::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp->GetAnimInstance()->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
	{
		IIAnimInstance::Execute_ResetTurn(MeshComp->GetAnimInstance());
	}
}