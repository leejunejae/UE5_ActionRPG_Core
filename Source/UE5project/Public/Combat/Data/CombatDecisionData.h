// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "Utils/CoreLog.h"
#include "CombatDecisionData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ECombatActionType : uint8
{
    None    UMETA(DisplayName = "None"),
    Attack  UMETA(DisplayName = "Attack"),
    Defend   UMETA(DisplayName = "Defend"),
    Evasion   UMETA(DisplayName = "Evasion"),
    Chase   UMETA(DisplayName = "Chase"),
    Alert   UMETA(DisplayName = "Alert"),
    Recover   UMETA(DisplayName = "Recover"),
};

USTRUCT(BlueprintType)
struct FCombatContext
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
        float DistanceToTarget = 0.f;

    UPROPERTY() 
        float AbsDeltaYawDeg = 0.f;

    UPROPERTY(BlueprintReadWrite)
        float HPPercent = 1.f;

    UPROPERTY(BlueprintReadWrite)
        int32 Phase = 1;

    UPROPERTY(BlueprintReadWrite)
        bool bPoiseBroken = false;

    UPROPERTY(BlueprintReadWrite)
        bool bStanceBroken = false;

    UPROPERTY(BlueprintReadWrite)
        float Aggressiveness = 0.5f; // 0~1

        // 타겟/상황 플래그들 (네 시스템에 맞게 채워)
    UPROPERTY() 
        bool bHasLOS = true;

    UPROPERTY() 
        bool bTargetInRecovery = false;

    UPROPERTY() 
        bool bTargetGuarding = false;

    UPROPERTY() 
        bool bTargetAttacking = false;

    UPROPERTY() 
        bool bTargetPoiseBroken = false;

    UPROPERTY() 
        bool bTargetStanceBroken = false;

    UPROPERTY() 
        bool bRangedThreat = false;

    UPROPERTY(BlueprintReadWrite)
        float CurrentTime = 0.f;
};

USTRUCT(BlueprintType)
struct FPatternCondition
{
    GENERATED_BODY()

        // Range
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        bool bUseRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bUseRange"))
        float MinRange = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bUseRange"))
        float MaxRange = 99999.f;

    UPROPERTY(EditAnywhere) 
        bool bUseIdealRange = false;

    UPROPERTY(EditAnywhere) 
        float IdealRange = 0.f; // bUseIdealRange=false면 (Min+Max)/2 사용

    // Angle
    UPROPERTY(EditAnywhere) 
        bool bUseAngle = false;

    UPROPERTY(EditAnywhere) 
        float AngleGoodDeg = 30.f; // 이내면 1

    UPROPERTY(EditAnywhere) 
        float AngleBadDeg = 90.f; // 이상이면 0

    // HP
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        bool bUseHPRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bUseHPRange"))
        float MinHPPercent = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bUseHPRange"))
        float MaxHPPercent = 1.f;

    // Phase
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        bool bUsePhase = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bUsePhase"))
        int32 RequiredPhase = 1;

    // Poise / Stance
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        bool bRequirePoiseBroken = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        bool bRequireStanceBroken = false;

    // Ranged threat
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        bool bRequireRangedThreat = false;

    // Cooldown
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float Cooldown = 0.f;

    bool IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const
    {
        if (Cooldown > 0.f)
        {
            const float Elapsed = Ctx.CurrentTime - LastUsedTime;
            if (Elapsed < Cooldown) return false;
        }
        
        if (bUsePhase)
        {
            if (Ctx.Phase < RequiredPhase) return false;
        }

        if (bUseRange)
        {
            if (Ctx.DistanceToTarget < MinRange || Ctx.DistanceToTarget > MaxRange) return false;
        }

        if (bUseHPRange)
        {
            if (Ctx.HPPercent < MinHPPercent || Ctx.HPPercent > MaxHPPercent) return false;
        }

        if (bRequirePoiseBroken)
        {
            if (!Ctx.bPoiseBroken) return false;
        }

        if (bRequireRangedThreat)
        {
            if (!Ctx.bRangedThreat) return false;
        }

        if (bRequireStanceBroken)
        {
            if (!Ctx.bStanceBroken) return false;
        }
        
        return true;
    }
};

USTRUCT(BlueprintType)
struct FCombatPattern : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FName PatternID; // 공격 이름, 몽타주 키 등

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        ECombatActionType ActionType = ECombatActionType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FPatternCondition Conditions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float BaseScore = 1.f; // 기본 가중치

        // 런타임 관리용

public:

    bool IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const
    {
        return Conditions.IsAvailable(Ctx, LastUsedTime);
    }

    float CalcScore(const FCombatContext& Ctx) const;
};

USTRUCT(BlueprintType)
struct FCombatDecisionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) ECombatActionType PickedType = ECombatActionType::None;
    UPROPERTY(BlueprintReadOnly) FName PickedPatternID = NAME_None;

    // 디버그용
    UPROPERTY(BlueprintReadOnly) TMap<ECombatActionType, float> TypeScores;
    UPROPERTY(BlueprintReadOnly) TMap<FName, float> PatternScores;
};

struct FEvalPattern
{
    const FCombatPattern* Row = nullptr;
    float Score = 0.f;
};

UCLASS()
class UE5PROJECT_API UCombatDecisionData : public UObject
{
	GENERATED_BODY()
	
};
