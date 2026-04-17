// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NativeGameplayTags.h"
#include "GameplayTagContainer.h"
#include "IKData.generated.h"

/**
 * 
 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase_Ground)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase_Ladder)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase_Ride)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ground)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ground_Locomotion)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ground_HandWeapon)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ladder)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ladder_Climb)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ride)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ride_Locomotion)


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


USTRUCT(BlueprintType)
struct FIKContextWeights
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = IK)
		TMap<ELimbList, float> LimbWeights;
	/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float HandL = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float HandR = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float FootL = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float FootR = 0.f;

	void Set(ELimbList Limb, float Weight)
	{
		Weight = FMath::Clamp(Weight, 0.f, 1.f);
		switch (Limb)
		{
		case ELimbList::HandL: HandL = Weight; break;
		case ELimbList::HandR: HandR = Weight; break;
		case ELimbList::FootL: FootL = Weight; break;
		case ELimbList::FootR: FootR = Weight; break;
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
		default: return 0.f;
		}
	}
	*/
};

UCLASS()
class UE5PROJECT_API UIKData : public UObject
{
	GENERATED_BODY()
	
};
