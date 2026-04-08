// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Climb/AN_EnterLadderFloat.h"
#include "Interaction/Climb/Components/ClimbComponent.h"

void UAN_EnterLadderFloat::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
    {
        if (auto* Climb = Owner->FindComponentByClass<UClimbComponent>())
        {
            Climb->EnterLadderFloat(); // 신호만
        }
    }
}