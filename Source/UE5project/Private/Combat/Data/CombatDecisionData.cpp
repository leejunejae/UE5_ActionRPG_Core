// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/CombatDecisionData.h"
#include "Utils/CustomMathUtility.h"

using namespace CustomMath::Math;

// ---------- FPatternCondition ----------

bool FPatternCondition::IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const
{
    // Cooldown
    if (Cooldown > 0.f)
    {
        const float Elapsed = Ctx.CurrentTime - LastUsedTime;
        if (Elapsed < Cooldown) return false;
    }

    // Phase
    if (bUsePhase && Ctx.Phase < RequiredPhase)
    {
        return false;
    }

    // Range
    if (bUseRange)
    {
        if (Ctx.DistanceToTarget < MinRange || Ctx.DistanceToTarget > MaxRange)
        {
            return false;
        }
    }

    // HP Range
    if (bUseHPRange)
    {
        if (Ctx.HPPercent < MinHPPercent || Ctx.HPPercent > MaxHPPercent)
        {
            return false;
        }
    }

    // Status flags
    if (bRequirePoiseBroken && !Ctx.bPoiseBroken)        return false;
    if (bRequireStanceBroken && !Ctx.bStanceBroken)      return false;
    if (bRequireRangedThreat && !Ctx.bRangedThreat)      return false;

    return true;
}

// ---------- FCombatPattern ----------

float FCombatPattern::CalcScore(const FCombatContext& Ctx) const
{
    float Score = FMath::Max(0.f, BaseScore);
    const FPatternCondition& Cond = Conditions;

    // ---------- 거리(Shape + Gate) ----------
    if (Cond.bUseRange)
    {
        const float Min = Cond.MinRange;
        const float Max = Cond.MaxRange;

        // 경계 Feather는 컴포넌트 튜닝값을 쓰는 게 맞지만,
        // Row 함수는 독립적이니 여기선 “기본 30cm”로 두고,
        // 실제로는 바깥에서 Multiply하는 방식도 가능.
        const float Feather = 30.f;
        const float Gate = SoftRange(Ctx.DistanceToTarget, Min, Max, Feather);

        float Ideal = Cond.bUseIdealRange ? Cond.IdealRange : (Min + Max) * 0.5f;
        Ideal = FMath::Clamp(Ideal, Min + 1.f, Max - 1.f);

        const float Shape = Tent(Ctx.DistanceToTarget, Min, Ideal, Max);

        // Gate가 0이면 사실상 불가능
        Score *= Gate;

        // Shape(최적거리)에 따른 가중
        Score *= FMath::Lerp(0.25f, 1.0f, Shape);
    }

    // ---------- 각도 ----------
    if (Cond.bUseAngle)
    {
        const float UAngle = AngleWindowNormal(Ctx.AbsDeltaYawDeg, Cond.AngleGoodDeg, Cond.AngleBadDeg);
        Score *= FMath::Lerp(0.2f, 1.0f, UAngle);
    }

    // ---------- LOS ----------
    if (!Ctx.bHasLOS)
    {
        switch (ActionType)
        {
        case ECombatActionType::Attack:  Score *= 0.15f; break;
        case ECombatActionType::Recover: Score *= 0.9f;  break;
        case ECombatActionType::Chase:   Score *= 1.1f;  break;
        case ECombatActionType::Alert:   Score *= 1.3f;  break;
        default:                         Score *= 0.7f;  break;
        }
    }

    // ---------- 상대 윈도우 ----------
    if (ActionType == ECombatActionType::Attack)
    {
        if (Ctx.bTargetInRecovery) Score *= 1.35f;
        if (Ctx.bTargetGuarding)   Score *= 1.15f;
        if (Ctx.bTargetPoiseBroken || Ctx.bTargetStanceBroken) Score *= 1.4f;
    }

    if (Ctx.bTargetAttacking)
    {
        if (ActionType == ECombatActionType::Defend)  Score *= 1.25f;
        if (ActionType == ECombatActionType::Evasion) Score *= 1.35f;
    }

    // ---------- 내 리소스(HP) ----------
    switch (ActionType)
    {
    case ECombatActionType::Recover:
        Score *= FMath::Lerp(0.9f, 1.4f, 1.f - Ctx.HPPercent);
        break;

    case ECombatActionType::Chase:
        // 멀수록 올라가고, 너무 가까우면 내려가게
        Score *= FMath::Lerp(0.7f, 1.6f, RampNormal(Ctx.DistanceToTarget, 300.f, 900.f));
        if (Ctx.DistanceToTarget < 250.f) Score *= 0.2f;
        break;
    default:
        break;
    }

    return FMath::Max(0.f, Score);
}