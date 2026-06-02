// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/CombatPatternCondition.h"

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