// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/BlendSpace1D.h"
#include "Core/MovementTypes.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h" 
#include "AnimData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FPhaseAnimByCardinal
{
    GENERATED_BODY()

public:
        // 방향별 Start/Stop (필요한 쪽만 채워도 OK)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TMap<EDirection8Way, TObjectPtr<UAnimSequence>> StartAnims;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TMap<EDirection8Way, TObjectPtr<UAnimSequence>> StopAnims;
};

class UHitReactionDataAsset;

USTRUCT(BlueprintType)
struct FAnimDataSet
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UBlendSpace> Locomotion_CycleBS;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UHitReactionDataAsset> HitReactionAnimSet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UAnimMontage> DeathMontage;
};

USTRUCT(BlueprintType)
struct FAnimProfile
{
    GENERATED_BODY()
public:
        // 이 프로파일이 적용되기 위한 조건 태그(부분집합 매칭)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FGameplayTagContainer MatchTags;

    // 실제 애니메이션 셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FAnimDataSet AnimDataSet;
};


USTRUCT(Atomic, BlueprintType)
struct FPlayerAnimSet
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UBlendSpace1D> Locomotion_Normal_CycleBS;             // 무기별 보행/달리기 블렌드스페이스
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UBlendSpace> Locomotion_Combat_Forward_BS;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UBlendSpace> Locomotion_Combat_Backward_BS;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Locomotion_Idle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Locomotion_Start;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Locomotion_Stop_Jog;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Locomotion_Stop_Run;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Jump_Start;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Jump_Loop;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Fall_Loop;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Land_Jump;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Land_Fall;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Land_Jog;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Land_High;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> HitAir_Start;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> HitAir_Loop;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> HitAir_End;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> GetUp;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UAnimSequence> Guard;

    UPROPERTY(EditDefaultsOnly, Category = "Death") 
        TSoftObjectPtr<UAnimMontage> GroundDeathMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Death") 
        TSoftObjectPtr<UAnimMontage> AirDeathMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Death") 
        TSoftObjectPtr<UAnimMontage> LadderDeathMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Death") 
        TSoftObjectPtr<UAnimMontage> RideDeathMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
        TSoftObjectPtr<UAnimMontage> SpawnMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK")
        bool bUseWeaponIK = false;
};

UCLASS()
class UE5PROJECT_API UAnimData : public UObject
{
	GENERATED_BODY()
	
};
