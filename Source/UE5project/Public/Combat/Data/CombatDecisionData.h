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
    Reposition   UMETA(DisplayName = "Reposition"),
    Alert   UMETA(DisplayName = "Alert"),
    Recover   UMETA(DisplayName = "Recover"),
};

/**
 * 전투 의사결정에 필요한 런타임 컨텍스트
 */
USTRUCT(BlueprintType)
struct FCombatContext
{
    GENERATED_BODY()

public:
    /** 타겟까지의 거리 (cm) */
    UPROPERTY(BlueprintReadWrite, Category = "Spatial")
    float DistanceToTarget = 0.f;

    /** 타겟 방향과의 절대 Yaw 차이 (도) */
    UPROPERTY(BlueprintReadWrite, Category = "Spatial")
    float AbsDeltaYawDeg = 0.f;

    /** 시야 확보 여부 */
    UPROPERTY(BlueprintReadWrite, Category = "Spatial")
    bool bHasLOS = true;

    /** 자기 HP 비율 0~1 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    float HPPercent = 1.f;

    /** 현재 페이즈 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    int32 Phase = 1;

    /** 자기 경직 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    bool bPoiseBroken = false;

    /** 자기 가드 붕괴 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    bool bStanceBroken = false;

    /** 공격 성향 0~1 (캐릭터 personality에서 주입) */
    UPROPERTY(BlueprintReadWrite, Category = "Personality")
    float Aggressiveness = 0.5f;

    /** 타겟이 공격 후딜 중인가 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetInRecovery = false;

    /** 타겟이 가드 중인가 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetGuarding = false;

    /** 타겟이 공격 모션 중인가 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetAttacking = false;

    /** 타겟 경직 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetPoiseBroken = false;

    /** 타겟 가드 붕괴 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetStanceBroken = false;

    /** 원거리 위협 존재 여부 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bRangedThreat = false;

    /** 현재 시간 (월드 시간) */
    UPROPERTY(BlueprintReadWrite, Category = "Time")
    float CurrentTime = 0.f;
};

/**
 * 패턴 사용 가능 여부를 판정하는 하드 조건들
 */
USTRUCT(BlueprintType)
struct FPatternCondition
{
    GENERATED_BODY()

    // ---------- Range ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    bool bUseRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range", meta = (EditCondition = "bUseRange"))
    float MinRange = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range", meta = (EditCondition = "bUseRange"))
    float MaxRange = 99999.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    bool bUseIdealRange = false;

    /** bUseIdealRange=false면 (Min+Max)/2 사용 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range", meta = (EditCondition = "bUseIdealRange"))
    float IdealRange = 0.f;

    // ---------- Angle ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Angle")
    bool bUseAngle = false;

    /** 이 각도 이내면 점수 1 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Angle", meta = (EditCondition = "bUseAngle"))
    float AngleGoodDeg = 30.f;

    /** 이 각도 이상이면 점수 0 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Angle", meta = (EditCondition = "bUseAngle"))
    float AngleBadDeg = 90.f;

    // ---------- HP ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HP")
    bool bUseHPRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HP", meta = (EditCondition = "bUseHPRange", ClampMin = "0.0", ClampMax = "1.0"))
    float MinHPPercent = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HP", meta = (EditCondition = "bUseHPRange", ClampMin = "0.0", ClampMax = "1.0"))
    float MaxHPPercent = 1.f;

    // ---------- Phase ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
    bool bUsePhase = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase", meta = (EditCondition = "bUsePhase", ClampMin = "1"))
    int32 RequiredPhase = 1;

    // ---------- Status Flags ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    bool bRequirePoiseBroken = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    bool bRequireStanceBroken = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    bool bRequireRangedThreat = false;

    // ---------- Cooldown ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (ClampMin = "0.0"))
    float Cooldown = 0.f;

    bool IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const;
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
