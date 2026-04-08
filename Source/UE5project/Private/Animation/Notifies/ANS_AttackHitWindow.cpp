// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/ANS_AttackHitWindow.h"
#include "Characters/CharacterBase.h"
#include "Combat/Components/AttackComponent.h"
#include "Core/Subsystems/GameInstanceSystem/AnimBoneDataSubsystem.h"
#include "GameplayTags.h"

void UANS_AttackHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp) return;

    ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
    if (!Character) return;
    if (UAttackComponent* AttackComp = Character->GetAttackComponent())
    {
        if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
        {
            PrevTime = Notify->GetTriggerTime();
            EndTime = Notify->GetEndTriggerTime();
            bFinished = false;
            
            FGameplayTag ProfileTag = Character->GetCharacterProfileTag();
            const UAnimSequence* Seq = Cast<UAnimSequence>(Anim);
            AttackComp->BeginAttackTrace(ProfileTag, Seq, WindowName);

            UE_LOG(LogTemp, Error, TEXT("StartTime : %f"), PrevTime);
        }
    }
}

void UANS_AttackHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    if (bFinished) return;

    if (!MeshComp) return;

    ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
    if (!Character) return;

    if (UAttackComponent* AttackComp = Character->GetAttackComponent())
    {
        if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
        {
            float CurrentTime = PrevTime + FrameDeltaTime;
            if (CurrentTime < Notify->GetEndTriggerTime())
            {
                AttackComp->ExecuteAttackTrace(PrevTime, CurrentTime, bDrawDebug);
                UE_LOG(LogTemp, Error, TEXT("PrevTime : %f, CurrentTime : %f"), PrevTime, CurrentTime);
                PrevTime = CurrentTime;
            }
            else
            {
                AttackComp->ExecuteAttackTrace(PrevTime, EndTime, bDrawDebug);
                bFinished = true;
                UE_LOG(LogTemp, Error, TEXT("PrevTime : %f, EndTime : %f"), PrevTime, EndTime);
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
            if (!bFinished)
            {
                bFinished = true;
                AttackComp->ExecuteAttackTrace(PrevTime, EndTime, bDrawDebug);
                UE_LOG(LogTemp, Error, TEXT("Notify End PrevTime : %f, EndTime :%f"), PrevTime, EndTime);
            }
        }
    }
}