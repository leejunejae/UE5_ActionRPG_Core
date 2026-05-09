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

    // 이 메시컴포넌트의 누적 시간을 0으로 초기화
    float StartTime = 0.f;

    if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
    {
        StartTime = Notify->GetTriggerTime();
    }

    ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
    if (!Character) return;

    if (UAttackComponent* AttackComp = Character->GetAttackComponent())
    {
        FGameplayTag ProfileTag = Character->GetCharacterProfileTag();
        const UAnimSequence* Seq = Cast<UAnimSequence>(Anim);
        AttackComp->BeginAttackTrace(ProfileTag, Seq, WindowName, StartTime);
    }
}

void UANS_AttackHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp) return;

    ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner());
    if (!Character) return;

    if (UAttackComponent* AttackComp = Character->GetAttackComponent())
    {
        AttackComp->TickAttackTrace(FrameDeltaTime, bDrawDebug);
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