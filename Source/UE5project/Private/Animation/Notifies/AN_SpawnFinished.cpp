// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_SpawnFinished.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Components/CharacterStatusComponent.h"

void UAN_SpawnFinished::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)return;

	ACharacterBase* Owner = Cast<ACharacterBase>(MeshComp->GetOwner());
	if (Owner)
	{
		if (UCharacterStatusComponent* StatusComp = Owner->GetCharacterStatusComponent())
			StatusComp->FinalizeRespawn();
	}
}