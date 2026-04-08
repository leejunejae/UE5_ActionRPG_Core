// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Climb/AN_ExitLadderFloat.h"
#include "Interaction/Climb/Components/ClimbComponent.h"


void UAN_ExitLadderFloat::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
    {
        if (auto* Climb = Owner->FindComponentByClass<UClimbComponent>())
        {
            Climb->ExitLadderFloat(); // 신호만
        }
    }
}