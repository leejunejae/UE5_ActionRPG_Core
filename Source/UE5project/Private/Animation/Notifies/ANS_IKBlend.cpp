// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/ANS_IKBlend.h"
#include "Utils/CoreLog.h"
#include "Utils/CustomMathUtility.h"
#include "Animation/Interfaces/IAnimInstance.h"
#include "Animation/AnimNotifyLibrary.h"

void UANS_IKBlend::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (MeshComp && MeshComp->GetAnimInstance() && MeshComp->GetAnimInstance()->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
    {
        if (!bInitAlphaValue)
            return;

        float TargetAlpha = bAlphaToZero ? 1.0f : 0.0f;

        switch (Mode)
        {
        case EIKConvertMode::Phase:
        {
            IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), ToPhaseTag, TargetAlpha);
            IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), FromPhaseTag, 1.0f - TargetAlpha);
            break;
        }
        case EIKConvertMode::Layer:
        {
            IIAnimInstance::Execute_SetIKLayerAlpha(MeshComp->GetAnimInstance(), LayerTag, TargetLimb, TargetAlpha);
            break;
        }
        }
    }
}

void UANS_IKBlend::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    if (MeshComp && MeshComp->GetAnimInstance() && MeshComp->GetAnimInstance()->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
    {
        if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
        {
            const float StartTime = Notify->GetTriggerTime();
            const float EndTime = Notify->GetEndTriggerTime();
            const float CurrentTime = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTime(EventReference);

            const float CurrentRatio = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTimeRatio(EventReference);

            const float InterpDuration = FMath::Max(EndTime - StartTime, KINDA_SMALL_NUMBER);
            
            const float ElapseRatio = FMath::Clamp(CurrentTime - StartTime / InterpDuration, 0.0f, 1.0f);

            const float OutAlpha = bAlphaToZero ? 1.0f - ApplyCurve(CurrentRatio, BlendMode) : ApplyCurve(CurrentRatio, BlendMode);

            float CurrentAlpha;

            CurrentAlpha = Mode == EIKConvertMode::Phase
                ? IIAnimInstance::Execute_GetIKPhaseAlpha(MeshComp->GetAnimInstance(), ToPhaseTag)
                : IIAnimInstance::Execute_GetIKLayerAlpha(MeshComp->GetAnimInstance(), LayerTag, TargetLimb);

            /*
            if (TargetLimb == ELimbList::FootL && LayerTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("IK.Layer.Ladder.Climb"))))
                UE_LOG(Log_Anim_IK, Warning, TEXT("Mode = %s, Ratio=%.4f"),
                    *UEnum::GetValueAsString(Mode), CurrentRatio);
                    * */

            if (!bAlphaToZero ? OutAlpha <= CurrentAlpha : OutAlpha >= CurrentAlpha)
                return;


            switch (Mode)
            {
            case EIKConvertMode::Phase:
            {
                IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), ToPhaseTag, OutAlpha);
                IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), FromPhaseTag, 1.0f - OutAlpha);
                //UE_LOG(Log_Anim_IK, Log, TEXT("[ANS_IKBlend] Phase Alpha [From %s : %f, To %s : %f]"), *FromPhaseTag.ToString(), 1.0f - OutAlpha, *ToPhaseTag.ToString(), OutAlpha);
                break;
            }
            case EIKConvertMode::Layer:
            {
                IIAnimInstance::Execute_SetIKLayerAlpha(MeshComp->GetAnimInstance(), LayerTag, TargetLimb, OutAlpha);

                if(TargetLimb == ELimbList::FootL && LayerTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("IK.Layer.Ladder.Climb"))))
                    //UE_LOG(Log_Anim_IK, Log, TEXT("[ANS_IKBlend] Layer Alpha : %f"), *LayerTag.ToString(), OutAlpha);
                break;
            }
            }
        }
    }
}

void UANS_IKBlend::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference)
{
    if (MeshComp && MeshComp->GetAnimInstance() && MeshComp->GetAnimInstance()->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass()))
    {

        float TargetAlpha = bAlphaToZero ? 0.0f : 1.0f;

        switch (Mode)
        {
        case EIKConvertMode::Phase:
        {
            IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), ToPhaseTag, TargetAlpha);
            IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), FromPhaseTag, 1.0f - TargetAlpha);
            //UE_LOG(Log_Anim_IK, Log, TEXT("[ANS_IKBlend] Phase Alpha [From %s : %f, To %s : %f]"), *FromPhaseTag.ToString(), 1.0f - TargetAlpha, *ToPhaseTag.ToString(), TargetAlpha);
            break;
        }
        case EIKConvertMode::Layer:
        {
            IIAnimInstance::Execute_SetIKLayerAlpha(MeshComp->GetAnimInstance(), LayerTag, TargetLimb, TargetAlpha);
           // UE_LOG(Log_Anim_IK, Log, TEXT("[ANS_IKBlend] Layer : %s, TargetLimb : %s, Alpha : %f"), *LayerTag.ToString(), *UEnum::GetValueAsString(TargetLimb), TargetAlpha);
            break;
        }
        }
    }
}

float UANS_IKBlend::ApplyCurve(float T, EBlendCurve Curve)
{
    using namespace CustomMath::BlendCurve;

    switch (Curve)
    {
    case EBlendCurve::EaseIn:         return EaseIn(T);
    case EBlendCurve::EaseOut:        return EaseOut(T);
    case EBlendCurve::EaseInOut:      return EaseInOut(T);
    case EBlendCurve::EaseInOutCubic: return EaseInOutCubic(T);
    case EBlendCurve::ElasticOut:     return ElasticOut(T);
    case EBlendCurve::BounceOut:      return BounceOut(T);
    default:                            return FMath::Clamp(T, 0.f, 1.f);
    }
}
