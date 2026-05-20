// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/CombatDecisionData.h"
#include "Utils/CustomMathUtility.h"

using namespace CustomMath::Math;

// ---------- FPatternCondition ----------

bool FPatternCondition::IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const
{
    // 하드 조건 체크
    if (bUseRange && !RangeCondition.PassesHardCheck(Ctx.DistanceToTarget))
        return false;

    if (bUseAngle && !AngleCondition.PassesHardCheck(Ctx.AbsDeltaYawDeg))
        return false;

    if (bUseHPRange && !HPCondition.PassesHardCheck(Ctx.HPPercent))
        return false;

    // 쿨다운
    if (Cooldown > 0.f && (Ctx.CurrentTime - LastUsedTime) < Cooldown)
        return false;

    // Phase
    if (bUsePhase && Ctx.Phase < RequiredPhase)
        return false;

    // Status flags
    if (bRequirePoiseBroken && !Ctx.bPoiseBroken)        return false;
    if (bRequireStanceBroken && !Ctx.bStanceBroken)      return false;
    if (bRequireRangedThreat && !Ctx.bRangedThreat)      return false;

    return true;
}

float FPatternCondition::CalcScore(const FCombatContext& Ctx) const
{
    // 각 조건의 (점수 × 가중치)를 누적, 가중치 합으로 정규화
    float TotalScore = 0.f;
    float TotalWeight = 0.f;

    if (bUseRange)
    {
        const float S = RangeCondition.CalcScore(Ctx.DistanceToTarget);
        TotalScore += S * RangeCondition.Weight;
        TotalWeight += RangeCondition.Weight;
    }

    if (bUseAngle)
    {
        const float S = AngleCondition.CalcScore(Ctx.AbsDeltaYawDeg);
        TotalScore += S * AngleCondition.Weight;
        TotalWeight += AngleCondition.Weight;
    }

    if (bUseHPRange)
    {
        const float S = HPCondition.CalcScore(Ctx.HPPercent);
        TotalScore += S * HPCondition.Weight;
        TotalWeight += HPCondition.Weight;
    }

    // 조건 하나도 없으면 1.0 (항상 적합)
    return TotalWeight > KINDA_SMALL_NUMBER ? (TotalScore / TotalWeight) : 1.0f;
}

// ---------- FCombatPattern ----------

float FCombatPattern::CalcScore(const FCombatContext& Ctx) const
{
    // 1. 조건 적합도 (0~1)
    const float ConditionScore = Conditions.CalcScore(Ctx);
    if (ConditionScore <= KINDA_SMALL_NUMBER)
    {
        return 0.f; // 조건 적합도가 0이면 최종 점수도 0
    }

    // 2. BaseScore 적용
    float Score = FMath::Max(0.f, BaseScore) * ConditionScore;

    // 3. 상황 보너스
    Score *= CalcSituationalMultiplier(Ctx);

    return FMath::Max(0.f, Score);
}

float FCombatPattern::CalcSituationalMultiplier(const FCombatContext& Ctx) const
{
    float Mult = 1.0f;

    // ---------- LOS ----------
    if (!Ctx.bHasLOS)
    {
        switch (ActionType)
        {
        case ECombatActionType::Attack:     Mult *= 0.15f; break;
        case ECombatActionType::Recover:    Mult *= 0.9f;  break;
        case ECombatActionType::Reposition: Mult *= 1.1f;  break;
        case ECombatActionType::Alert:      Mult *= 1.3f;  break;
        default:                            Mult *= 0.7f;  break;
        }
    }

    // ---------- 타겟 상태에 따른 ActionType 보너스 ----------
    if (ActionType == ECombatActionType::Attack)
    {
        if (Ctx.bTargetInRecovery) Mult *= 1.35f;
        if (Ctx.bTargetGuarding)   Mult *= 1.15f;
        if (Ctx.bTargetPoiseBroken || Ctx.bTargetStanceBroken) Mult *= 1.4f;
    }

    if (Ctx.bTargetAttacking)
    {
        if (ActionType == ECombatActionType::Defend)  Mult *= 1.25f;
        if (ActionType == ECombatActionType::Evasion) Mult *= 1.35f;
    }

    // ---------- 자기 리소스(HP) ----------
    switch (ActionType)
    {
    case ECombatActionType::Recover:
        // HP 낮을수록 더 선호
        Mult *= FMath::Lerp(0.9f, 1.4f, 1.f - Ctx.HPPercent);
        break;
    case ECombatActionType::Attack:
        if (Ctx.bTargetInRecovery) Mult *= 1.35f;
        if (Ctx.bTargetGuarding)   Mult *= 1.15f;
        if (Ctx.bTargetPoiseBroken || Ctx.bTargetStanceBroken) Mult *= 1.4f;
        break;
    case ECombatActionType::Defend:
        if (Ctx.bTargetAttacking) Mult *= 1.25f;
        break;
    case ECombatActionType::Evasion:
        if (Ctx.bTargetAttacking) Mult *= 1.35f;
        break;
    default:
        break;
    }

    return Mult;
}

float FRangeCondition::CalcScore(float Distance) const
{
    // 범위 밖
    if (Distance < MinRange || Distance > MaxRange)
    {
        return bHardFail ? 0.f : 0.1f; // 하드 실패는 0, 소프트는 살짝 점수
    }

    switch (ScoreMode)
    {
    case EScoreMode::Binary:
        return 1.0f;

    case EScoreMode::PeakAtIdeal:
    {
        // Ideal 지점에서 1, Min/Max 경계에서 0
        const float Ideal = IdealRange;
        const float MaxDist = FMath::Max(Ideal - MinRange, MaxRange - Ideal);
        if (MaxDist <= KINDA_SMALL_NUMBER) return 1.0f;

        const float DistFromIdeal = FMath::Abs(Distance - Ideal);
        return FMath::Clamp(1.0f - DistFromIdeal / MaxDist, 0.f, 1.f);
    }

    case EScoreMode::HigherIsBetter:
    {
        // Min에서 0, Max에서 1
                // ⭐ MinRange 미만: 점수 0 (또는 HardFail)
        if (Distance < MinRange) return bHardFail ? 0.f : 0.1f;

        // ⭐ MaxRange 이상: 점수 1.0 (천장 유지)
        if (Distance >= MaxRange) return 1.0f;

        const float Range = MaxRange - MinRange;
        if (Range <= KINDA_SMALL_NUMBER) return 1.0f;
        return FMath::Clamp((Distance - MinRange) / Range, 0.f, 1.f);
    }

    case EScoreMode::LowerIsBetter:
    {
        // ⭐ MinRange 미만: 점수 1.0 (바닥에서도 만점)
        if (Distance <= MinRange) return 1.0f;

        // ⭐ MaxRange 이상: 점수 0 (또는 HardFail)
        if (Distance > MaxRange) return bHardFail ? 0.f : 0.1f;

        // Min에서 1, Max에서 0
        const float Range = MaxRange - MinRange;
        if (Range <= KINDA_SMALL_NUMBER) return 1.0f;
        return FMath::Clamp(1.0f - (Distance - MinRange) / Range, 0.f, 1.f);
    }
    }

    return 1.0f;
}

bool FRangeCondition::PassesHardCheck(float Distance) const
{
    if (!bHardFail) return true; // 소프트 조건은 항상 통과
    
    switch (ScoreMode)
    {
    case EScoreMode::HigherIsBetter:
        // 너무 가까우면 실패, 멀면 항상 통과
        return Distance >= MinRange;

    case EScoreMode::LowerIsBetter:
        // 너무 멀면 실패, 가까우면 항상 통과
        return Distance <= MaxRange;

    case EScoreMode::Binary:
    case EScoreMode::PeakAtIdeal:
    default:
        return Distance >= MinRange && Distance <= MaxRange;
    }
}

float FAngleCondition::CalcScore(float AbsDeltaYawDeg) const
{
    if (AbsDeltaYawDeg <= AngleGoodDeg) return 1.0f;
    if (AbsDeltaYawDeg >= AngleBadDeg) return bHardFail ? 0.f : 0.1f;

    // Good~Bad 사이 보간 (1 → 0)
    const float T = (AbsDeltaYawDeg - AngleGoodDeg) / (AngleBadDeg - AngleGoodDeg);
    return FMath::Clamp(1.0f - T, 0.f, 1.f);
}

bool FAngleCondition::PassesHardCheck(float AbsDeltaYawDeg) const
{
    if (!bHardFail) return true;
    return AbsDeltaYawDeg <= AngleBadDeg;
}

bool FHPCondition::PassesHardCheck(float HPPercent) const
{
    if (!bHardFail) return true;
    return HPPercent >= MinHPPercent && HPPercent <= MaxHPPercent;
}

float FHPCondition::CalcScore(float HPPercent) const
{
    const bool bInRange = (HPPercent >= MinHPPercent && HPPercent <= MaxHPPercent);
    if (!bInRange)
    {
        return bHardFail ? 0.f : 0.1f;
    }

    switch (ScoreMode)
    {
    case EScoreMode::Binary:
        return 1.0f;

    case EScoreMode::PeakAtIdeal:
    {
        const float Ideal = FMath::Clamp(IdealHPPercent, MinHPPercent + 0.01f, MaxHPPercent - 0.01f);
        if (HPPercent < Ideal)
        {
            const float L = Ideal - MinHPPercent;
            return L > KINDA_SMALL_NUMBER ? (HPPercent - MinHPPercent) / L : 1.0f;
        }
        else
        {
            const float R = MaxHPPercent - Ideal;
            return R > KINDA_SMALL_NUMBER ? (MaxHPPercent - HPPercent) / R : 1.0f;
        }
    }

    case EScoreMode::HigherIsBetter:
    {
        const float Range = MaxHPPercent - MinHPPercent;
        return Range > KINDA_SMALL_NUMBER
            ? FMath::Clamp((HPPercent - MinHPPercent) / Range, 0.f, 1.f)
            : 1.0f;
    }

    case EScoreMode::LowerIsBetter:
    {
        // Recover에 적합: HP가 낮을수록 점수 ↑
        const float Range = MaxHPPercent - MinHPPercent;
        return Range > KINDA_SMALL_NUMBER
            ? FMath::Clamp(1.f - (HPPercent - MinHPPercent) / Range, 0.f, 1.f)
            : 1.0f;
    }
    }

    return 1.0f;
}