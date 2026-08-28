// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/BlendSpace1D.h"
#include "Core/MovementTypes.h"
#include "Characters/Data/StatusData.h"
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
class UAnimMontage;

UENUM(BlueprintType)
enum class ECriticalExecutionDirection : uint8
{
    Front UMETA(DisplayName = "Front"),
    Back UMETA(DisplayName = "Back"),
    Left UMETA(DisplayName = "Left"),
    Right UMETA(DisplayName = "Right")
};

USTRUCT(BlueprintType)
struct FCriticalExecutionAttackerEntry
{
    GENERATED_BODY()

    /** 공격자와 피격자 애니메이션 쌍을 연결하는 공통 식별자. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        FName ExecutionID = TEXT("Execution_01");

    /** 적의 시점을 기준으로 플레이어가 접근한 방향. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        ECriticalExecutionDirection Direction = ECriticalExecutionDirection::Front;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        TSoftObjectPtr<UAnimMontage> Montage;

    /** 대상 Transform 기준 플레이어의 시작 Transform. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        FTransform AttackerRelativeTransform =
            FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(100.0f, 0.0f, 0.0f));

    bool IsConfigured() const
    {
        return !ExecutionID.IsNone() && !Montage.IsNull();
    }
};

USTRUCT(BlueprintType)
struct FCriticalExecutionVictimEntry
{
    GENERATED_BODY()

    /** 대응하는 공격자 항목과 동일한 값을 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        FName ExecutionID = TEXT("Execution_01");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        ECriticalExecutionDirection Direction = ECriticalExecutionDirection::Front;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        TSoftObjectPtr<UAnimMontage> Montage;

    bool IsConfigured() const
    {
        return !ExecutionID.IsNone() && !Montage.IsNull();
    }
};

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

    /** 이 스켈레톤 프로필이 피격자로 재생할 동기 처형 애니메이션. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        TArray<FCriticalExecutionVictimEntry> CriticalExecutions;
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

    // 공통 회피를 기본으로 사용하고 무기별로 필요한 경우에만 덮어쓴다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Dodge")
        TSoftObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Parry")
        TSoftObjectPtr<UAnimMontage> ParryMontage;

    /** 현재 무기 애니메이션 세트가 공격자로 재생할 동기 처형 애니메이션. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Critical Execution")
        TArray<FCriticalExecutionAttackerEntry> CriticalExecutions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Dodge")
        FActionExitBlendSettings DodgeExitBlendSettings;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Parry")
        FActionExitBlendSettings ParryExitBlendSettings;

    // 음수면 CommonAnimSet의 값을 사용한다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Dodge", meta = (ClampMin = "-1.0", ClampMax = "0.5"))
        float DodgeLocomotionBlendOutTime = -1.0f;

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
