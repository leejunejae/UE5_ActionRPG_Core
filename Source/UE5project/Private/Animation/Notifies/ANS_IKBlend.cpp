// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/ANS_IKBlend.h"
#include "Utils/CustomMathUtility.h"
#include "Animation/Interfaces/IAnimInstance.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Animation/AnimMontage.h"

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
            const float InterpDuration = FMath::Max(Notify->GetDuration(), KINDA_SMALL_NUMBER);

            float CurrentRatio;
            if (const UAnimMontage* Montage = Cast<UAnimMontage>(Anim))
            {
                const float MontagePosition =
                    MeshComp->GetAnimInstance()->Montage_GetPosition(Montage);
                CurrentRatio = FMath::Clamp(
                    (MontagePosition - StartTime) / InterpDuration,
                    0.0f,
                    1.0f);
            }
            else
            {
                CurrentRatio =
                    UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTimeRatio(EventReference);
            }

            const float OutAlpha = bAlphaToZero ? 1.0f - ApplyCurve(CurrentRatio, BlendMode) : ApplyCurve(CurrentRatio, BlendMode);

            float CurrentAlpha;

            CurrentAlpha = Mode == EIKConvertMode::Phase
                ? IIAnimInstance::Execute_GetIKPhaseAlpha(MeshComp->GetAnimInstance(), ToPhaseTag)
                : IIAnimInstance::Execute_GetIKLayerAlpha(MeshComp->GetAnimInstance(), LayerTag, TargetLimb);

            if (!bAlphaToZero ? OutAlpha <= CurrentAlpha : OutAlpha >= CurrentAlpha)
                return;


            switch (Mode)
            {
            case EIKConvertMode::Phase:
            {
                IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), ToPhaseTag, OutAlpha);
                IIAnimInstance::Execute_SetIKPhaseAlpha(MeshComp->GetAnimInstance(), FromPhaseTag, 1.0f - OutAlpha);
                break;
            }
            case EIKConvertMode::Layer:
            {
                IIAnimInstance::Execute_SetIKLayerAlpha(MeshComp->GetAnimInstance(), LayerTag, TargetLimb, OutAlpha);
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
