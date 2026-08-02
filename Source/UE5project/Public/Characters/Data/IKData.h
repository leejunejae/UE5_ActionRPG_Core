// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "IKData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ELimbList : uint8 { HandL, HandR, FootL, FootR, Body };

UENUM(BlueprintType)
enum class EIKEase : uint8 { Linear, CubicInOut, ExpoInOut, CustomCurve };

USTRUCT()
struct FLimbAlphaMap
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere)
		TMap<ELimbList, float> Alphas;
};

USTRUCT(BlueprintType)
struct FIKLimbData 
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		ELimbList TargetLimb = ELimbList::HandL;

	UPROPERTY(EditAnywhere, Meta = (Categories = "IK.Phase"))
		FGameplayTag From;

	UPROPERTY(EditAnywhere, Meta = (Categories = "IK.Phase"))
		FGameplayTag To;

	// true면 목표 Alpha값이 0.0f, false면 목표 Alpha값이 1.0f
	UPROPERTY(EditAnywhere, Category = "IK") 
		bool bAlphaToZero = false;

	// 처음에 IK값을 초기화 할 것인가 bBlendIK 여부에 따라 1.Of, 0.0f로 초기화 할 것인가
	UPROPERTY(EditAnywhere, Category = "IK") 
		bool bInitAlphaValue = true;

	UPROPERTY(EditAnywhere)
		float StartTime;

	UPROPERTY(EditAnywhere)
		float EndTime;
};


/** Fixed runtime representation used by the AnimGraph. */
USTRUCT(BlueprintType)
struct FIKLimbWeights
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = IK)
	float HandL = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = IK)
	float HandR = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = IK)
	float FootL = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = IK)
	float FootR = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = IK)
	float Body = 0.0f;

	void Set(ELimbList Limb, float Weight)
	{
		const float ClampedWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
		switch (Limb)
		{
		case ELimbList::HandL: HandL = ClampedWeight; break;
		case ELimbList::HandR: HandR = ClampedWeight; break;
		case ELimbList::FootL: FootL = ClampedWeight; break;
		case ELimbList::FootR: FootR = ClampedWeight; break;
		case ELimbList::Body: Body = ClampedWeight; break;
		default: break;
		}
	}

	float Get(ELimbList Limb) const
	{
		switch (Limb)
		{
		case ELimbList::HandL: return HandL;
		case ELimbList::HandR: return HandR;
		case ELimbList::FootL: return FootL;
		case ELimbList::FootR: return FootR;
		case ELimbList::Body: return Body;
		default: return 0.0f;
		}
	}

};

UCLASS()
class UE5PROJECT_API UIKData : public UObject
{
	GENERATED_BODY()
	
};
