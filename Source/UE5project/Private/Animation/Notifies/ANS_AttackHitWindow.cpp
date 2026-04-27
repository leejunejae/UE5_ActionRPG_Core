// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/ANS_AttackHitWindow.h"
#include "Characters/CharacterBase.h"
#include "Combat/Components/AttackComponent.h"
#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "Animation/AnimNotifyLibrary.h"
#include "GameplayTags.h"
#include "Utils/CoreLog.h"

void UANS_AttackHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp) return;
    
    ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
    if (!Character) return;
    if (UAttackComponent* AttackComp = Character->GetAttackComponent())
    {
        if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
        {
            FGameplayTag ProfileTag = Character->GetCharacterProfileTag();
            const UAnimSequence* Seq = Cast<UAnimSequence>(Anim);
            AttackComp->BeginAttackTrace(ProfileTag, Seq, WindowName);
        }
    }
}

void UANS_AttackHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp) return;

    ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
    if (!Character) return;

    if (UAttackComponent* AttackComp = Character->GetAttackComponent())
    {
        if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
        {
            float CurrentTime = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTime(EventReference);
            float PrevTime = CurrentTime - FrameDeltaTime;
            if (CurrentTime < Notify->GetEndTriggerTime())
            {
                AttackComp->ExecuteAttackTrace(PrevTime, CurrentTime, bDrawDebug);
                UE_LOG(Log_Attack, Error, TEXT("PrevTime : %f, CurrentTime : %f"), PrevTime, CurrentTime);
                PrevTime = CurrentTime;
            }
            else
            {
                float EndTime = Notify->GetEndTriggerTime();
                AttackComp->ExecuteAttackTrace(PrevTime, EndTime, bDrawDebug);
                UE_LOG(Log_Attack, Error, TEXT("PrevTime : %f, EndTime : %f"), PrevTime, EndTime);
                PrevTime = EndTime;
            }    
        }
    }
}

void UANS_AttackHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp) return;

    ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
    if (!Character) return;

    if (UAttackComponent* AttackComp = Character->GetAttackComponent())
    {
        if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
        {
            float EndTime = Notify->GetEndTriggerTime();
            AttackComp->EndAttackTrace(EndTime, bDrawDebug);
        }
    }
}