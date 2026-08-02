// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/ANS_IKBlend.h"
#include "Utils/CustomMathUtility.h"
#include "Animation/Interfaces/IAnimInstance.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Animation/AnimMontage.h"

void UANS_IKBlend::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (UAnimInstance* AnimInstance = GetCompatibleAnimInstance(MeshComp))
    {
        ActiveBlends.FindOrAdd(AnimInstance) =
            CaptureStartAlphas(AnimInstance, bInitAlphaValue);
    }
}

void UANS_IKBlend::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    if (UAnimInstance* AnimInstance = GetCompatibleAnimInstance(MeshComp))
    {
        if (const FAnimNotifyEvent* Notify = EventReference.GetNotify())
        {
            const float StartTime = Notify->GetTriggerTime();
            const float InterpDuration = FMath::Max(Notify->GetDuration(), KINDA_SMALL_NUMBER);

            float CurrentRatio;
            if (const UAnimMontage* Montage = Cast<UAnimMontage>(Anim))
            {
                const float MontagePosition =
                    AnimInstance->Montage_GetPosition(Montage);
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

            FActiveIKBlend* Blend = ActiveBlends.Find(AnimInstance);
            if (!Blend)
            {
                Blend = &ActiveBlends.Add(
                    AnimInstance,
                    CaptureStartAlphas(AnimInstance, false));
            }

            ApplyBlendAlphas(
                AnimInstance,
                *Blend,
                ApplyCurve(CurrentRatio, BlendMode));
        }
    }
}

void UANS_IKBlend::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference)
{
    if (UAnimInstance* AnimInstance = GetCompatibleAnimInstance(MeshComp))
    {
        ApplyFinalAlphas(AnimInstance);
        ActiveBlends.Remove(AnimInstance);
    }
}

UAnimInstance* UANS_IKBlend::GetCompatibleAnimInstance(
    USkeletalMeshComponent* MeshComp) const
{
    UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
    return AnimInstance &&
        AnimInstance->GetClass()->ImplementsInterface(UIAnimInstance::StaticClass())
        ? AnimInstance
        : nullptr;
}

UANS_IKBlend::FActiveIKBlend UANS_IKBlend::CaptureStartAlphas(
    UAnimInstance* AnimInstance,
    bool bApplyInitialValue) const
{
    FActiveIKBlend Blend;
    const float InitialToOrLayerAlpha = bAlphaToZero ? 1.0f : 0.0f;

    if (Mode == EIKConvertMode::Phase)
    {
        Blend.ToOrLayerStartAlpha = bApplyInitialValue
            ? InitialToOrLayerAlpha
            : IIAnimInstance::Execute_GetIKPhaseAlpha(AnimInstance, ToPhaseTag);
        Blend.FromStartAlpha = bApplyInitialValue
            ? 1.0f - InitialToOrLayerAlpha
            : IIAnimInstance::Execute_GetIKPhaseAlpha(AnimInstance, FromPhaseTag);

        if (bApplyInitialValue)
        {
            IIAnimInstance::Execute_SetIKPhaseAlpha(
                AnimInstance, ToPhaseTag, Blend.ToOrLayerStartAlpha);
            IIAnimInstance::Execute_SetIKPhaseAlpha(
                AnimInstance, FromPhaseTag, Blend.FromStartAlpha);
        }
    }
    else
    {
        Blend.ToOrLayerStartAlpha = bApplyInitialValue
            ? InitialToOrLayerAlpha
            : IIAnimInstance::Execute_GetIKLayerAlpha(
                AnimInstance, LayerTag, TargetLimb);

        if (bApplyInitialValue)
        {
            IIAnimInstance::Execute_SetIKLayerAlpha(
                AnimInstance, LayerTag, TargetLimb,
                Blend.ToOrLayerStartAlpha);
        }
    }

    return Blend;
}

void UANS_IKBlend::ApplyBlendAlphas(
    UAnimInstance* AnimInstance,
    const FActiveIKBlend& Blend,
    float Progress) const
{
    const float TargetAlpha = bAlphaToZero ? 0.0f : 1.0f;
    const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
    const float ToOrLayerAlpha = FMath::Lerp(
        Blend.ToOrLayerStartAlpha, TargetAlpha, ClampedProgress);

    if (Mode == EIKConvertMode::Phase)
    {
        const float FromAlpha = FMath::Lerp(
            Blend.FromStartAlpha, 1.0f - TargetAlpha, ClampedProgress);
        IIAnimInstance::Execute_SetIKPhaseAlpha(
            AnimInstance, ToPhaseTag, ToOrLayerAlpha);
        IIAnimInstance::Execute_SetIKPhaseAlpha(
            AnimInstance, FromPhaseTag, FromAlpha);
    }
    else
    {
        IIAnimInstance::Execute_SetIKLayerAlpha(
            AnimInstance, LayerTag, TargetLimb, ToOrLayerAlpha);
    }
}

void UANS_IKBlend::ApplyFinalAlphas(UAnimInstance* AnimInstance) const
{
    const float TargetAlpha = bAlphaToZero ? 0.0f : 1.0f;
    if (Mode == EIKConvertMode::Phase)
    {
        IIAnimInstance::Execute_SetIKPhaseAlpha(
            AnimInstance, ToPhaseTag, TargetAlpha);
        IIAnimInstance::Execute_SetIKPhaseAlpha(
            AnimInstance, FromPhaseTag, 1.0f - TargetAlpha);
    }
    else
    {
        IIAnimInstance::Execute_SetIKLayerAlpha(
            AnimInstance, LayerTag, TargetLimb, TargetAlpha);
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
