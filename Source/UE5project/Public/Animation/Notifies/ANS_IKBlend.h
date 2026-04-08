// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Characters/Data/IKData.h"
#include "ANS_IKBlend.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EIKConvertMode : uint8 { Phase, Layer };

UENUM(BlueprintType)
enum class EBlendCurve : uint8
{
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseInOutCubic,
    ElasticOut,
    BounceOut,
};

UCLASS()
class UE5PROJECT_API UANS_IKBlend : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "IK")
		EIKConvertMode Mode;

	UPROPERTY(EditAnywhere, meta = (Categories = "IK.Phase", EditCondition = "Mode == EIKConvertMode::Phase", EditConditionHides))
		FGameplayTag FromPhaseTag;

	UPROPERTY(EditAnywhere, meta = (Categories = "IK.Phase", EditCondition = "Mode == EIKConvertMode::Phase", EditConditionHides))
		FGameplayTag ToPhaseTag;

	UPROPERTY(EditAnywhere, meta = (Categories = "IK.Layer", EditCondition = "Mode == EIKConvertMode::Layer", EditConditionHides))
		FGameplayTag LayerTag;

	UPROPERTY(EditAnywhere, meta = (Categories = "IK.Layer", EditCondition = "Mode == EIKConvertMode::Layer", EditConditionHides))
		ELimbList TargetLimb = ELimbList::HandL;

	UPROPERTY(EditAnywhere, Category = "IK")
		EBlendCurve BlendMode = EBlendCurve::Linear;

	UPROPERTY(EditAnywhere, Category = "IK")
		bool bAlphaToZero = false;

	// 처음에 IK값을 초기화 할 것인가 bBlendIK 여부에 따라 1.Of, 0.0f로 초기화 할 것인가
	UPROPERTY(EditAnywhere, Category = "IK")
		bool bInitAlphaValue = true;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, const FAnimNotifyEventReference& EventReference) override;

private:
    float ApplyCurve(float T, EBlendCurve Curve);

    float TotalLen = 0.f;
    float Elapsed = 0.f;
};
