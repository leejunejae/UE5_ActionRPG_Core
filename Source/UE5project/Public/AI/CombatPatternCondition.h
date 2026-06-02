// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Combat/Data/CombatDecisionData.h"
#include "CombatPatternCondition.generated.h"

/* ============================================================
 *  Pattern Evaluation Criteria for HardFailed / SoftScore
 * ============================================================ */

UENUM(BlueprintType)
enum class EScoreMode : uint8
{
    /** 조건 범위 안이면 1.0, 밖이면 0 */
    Binary          UMETA(DisplayName = "Binary"),

    /** Ideal 지점에 가까울수록 1, 경계로 갈수록 0 — Attack에 적합 */
    PeakAtIdeal     UMETA(DisplayName = "Peak at Ideal"),

    /** 값이 클수록 점수 ↑ (Min~Max 사이를 0~1로 보간) — Chase에 적합 */
    HigherIsBetter  UMETA(DisplayName = "Higher Is Better"),

    /** 값이 작을수록 점수 ↑ — Retreat이 가까운 거리에서 더 선호되도록 할 때 */
    LowerIsBetter   UMETA(DisplayName = "Lower Is Better"),
};

USTRUCT(BlueprintType)
struct FRangeCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    float MinRange = 0.f;

    UPROPERTY(EditAnywhere)
    float MaxRange = 99999.f;

    /** Range 점수화 방식 */
    UPROPERTY(EditAnywhere)
    EScoreMode ScoreMode = EScoreMode::Binary;

    /** PeakAtIdeal 모드일 때 사용 */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "ScoreMode == EScoreMode::PeakAtIdeal"))
    float IdealRange = 200.f;

    /** 이 조건의 점수가 최종 점수에 미치는 영향력 (가중치) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    /** Min/Max 밖이면 하드 실패인가, 그냥 점수 0인가 */
    UPROPERTY(EditAnywhere)
    bool bHardFail = true;

    bool PassesHardCheck(float Value) const;
    float CalcScore(float Value) const;
};

USTRUCT(BlueprintType)
struct FHPCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinHPPercent = 0.f;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxHPPercent = 1.f;

    UPROPERTY(EditAnywhere)
    EScoreMode ScoreMode = EScoreMode::Binary;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IdealHPPercent = 0.5f;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere)
    bool bHardFail = false; // HP는 보통 점수만 영향 (예외: Recover는 HP 낮을 때만)

    bool PassesHardCheck(float Value) const;
    float CalcScore(float Value) const;
};

USTRUCT(BlueprintType)
struct FAngleCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float AngleGoodDeg = 30.f; // 이 안이면 점수 1

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float AngleBadDeg = 90.f;  // 이 밖이면 점수 0

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere)
    bool bHardFail = false;

    bool PassesHardCheck(float Value) const;
    float CalcScore(float Value) const;
};

USTRUCT(BlueprintType)
struct FPatternCondition
{
    GENERATED_BODY()

    // ---------- Range ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    bool bUseRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range", meta = (EditCondition = "bUseRange", EditConditionHides))
    FRangeCondition RangeCondition;

    // ---------- Angle ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Angle")
    bool bUseAngle = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Angle", meta = (EditCondition = "bUseAngle", EditConditionHides))
    FAngleCondition AngleCondition;

    // ---------- HP ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HP")
    bool bUseHPRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HP", meta = (EditCondition = "bUseHPRange", EditConditionHides))
    FHPCondition HPCondition;

    // ---------- Phase ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
    bool bUsePhase = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase", meta = (EditCondition = "bUsePhase", EditConditionHides, ClampMin = "1"))
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
    float CalcScore(const FCombatContext& Ctx) const;
};
