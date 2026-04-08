// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_ResetHitAir.h"
#include "Animation/Interfaces/IAnimInstance.h"

void UAN_ResetHitAir::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp->GetAnimInstance()->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
	{
		IIAnimInstance::Execute_ResetHitAir(MeshComp->GetAnimInstance());
	}
}