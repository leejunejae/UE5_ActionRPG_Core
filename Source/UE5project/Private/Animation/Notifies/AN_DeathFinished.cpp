// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_DeathFinished.h"
#include "Combat/Interfaces/HitReactionInterface.h"

void UAN_DeathFinished::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)return;

	AActor* Owner = MeshComp->GetOwner();
	if (Owner && Owner->GetClass()->ImplementsInterface(UHitReactionInterface::StaticClass()))
	{
		IHitReactionInterface::Execute_OnDeathEnd(Owner);
	}
}