// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AN_SetPlayerGroundState.h"
#include "GameFramework/Character.h"

void UAN_SetPlayerGroundState::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp)
	{
		for (UActorComponent* Comp : MeshComp->GetOwner()->GetComponents())
		{
			if (Comp->GetClass()->ImplementsInterface(UCharacterStatusInterface::StaticClass()))
			{
				ICharacterStatusInterface::Execute_SetGroundStance(Comp, NewState);
			}
		}
	}
}