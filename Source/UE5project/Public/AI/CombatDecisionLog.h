// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Data/CombatDecisionData.h"
#include "Utils/CoreLog.h"

namespace CombatLog
{
    // ── 공통 유틸 ───────────────────────────────────────────────

    inline FString TypeName(ECombatActionType Type)
    {
        return StaticEnum<ECombatActionType>()->GetNameStringByValue((int64)Type);
    }

    inline void Separator()
    {
        UE_LOG(Log_AI, Log, TEXT("  ---------------------------------------------------------------"));
    }


    // ── Decide() 흐름 로그 ──────────────────────────────────────

    inline void DecideHeader(const FString& ActorName)
    {
        UE_LOG(Log_AI, Log, TEXT(" "));
        UE_LOG(Log_AI, Log, TEXT("[CombatDecision] EvaluatePattern | Actor: %s"), *ActorName);
        Separator();
    }

    inline void NullTable(const FString& ActorName)
    {
        UE_LOG(Log_AI, Warning, TEXT("[CombatDecision] PatternTable is null | Actor: %s"), *ActorName);
    }

    inline void NoValidCandidates()
    {
        UE_LOG(Log_AI, Warning, TEXT("[CombatDecision] No valid candidates -> fallback to Chase"));
    }

    inline void TypePickFailed()
    {
        UE_LOG(Log_AI, Warning, TEXT("[CombatDecision] TypePick failed -> fallback to Chase"));
    }


    // ── 패턴 평가 로그 (Verbose: 평소엔 숨김) ───────────────────

    inline void PatternHardFail(FName PatternID, const FString& TypeStr)
    {
        UE_LOG(Log_AI, Verbose, TEXT("  [Pattern] %-28s | Type: %-8s | Hard: FAIL"),
            *PatternID.ToString(), *TypeStr);
    }

    inline void PatternTooLowScore(FName PatternID, const FString& TypeStr, float Score)
    {
        UE_LOG(Log_AI, Verbose, TEXT("  [Pattern] %-28s | Type: %-8s | Hard: PASS | Score: %.3f (too low)"),
            *PatternID.ToString(), *TypeStr, Score);
    }

    inline void PatternPass(FName PatternID, const FString& TypeStr, float Score)
    {
        UE_LOG(Log_AI, Verbose, TEXT("  [Pattern] %-28s | Type: %-8s | Hard: PASS | Soft: PASS | Score: %.3f"),
            *PatternID.ToString(), *TypeStr, Score);
    }


    // ── DebugPrint() 결과 요약 ───────────────────────────────────

    inline void PrintResult(const FCombatDecisionResult& Result)
    {
        // 패턴 점수 -> 내림차순 정렬
        TArray<TPair<FName, float>> SortedPatterns(Result.PatternScores.Array());
        SortedPatterns.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B)
            {
                return A.Value > B.Value;
            });

        const float* PickedScorePtr = Result.PatternScores.Find(Result.PickedPatternID);
        const float  PickedScore = PickedScorePtr ? *PickedScorePtr : 0.f;

        // 패턴 점수 블록
        for (auto& Pair : SortedPatterns)
        {
            const bool bPicked = (Pair.Key == Result.PickedPatternID);
            UE_LOG(Log_AI, Log, TEXT("  [Pattern] %-28s | Score: %.3f%s"),
                *Pair.Key.ToString(),
                Pair.Value,
                bPicked ? TEXT("  <- PICKED") : TEXT(""));
        }

        Separator();

        // 타입 점수 블록
        for (auto& Pair : Result.TypeScores)
        {
            const bool bPickedType = (Pair.Key == Result.PickedType);
            UE_LOG(Log_AI, Log, TEXT("  [Type]    %-10s | Score: %.3f%s"),
                *TypeName(Pair.Key),
                Pair.Value,
                bPickedType ? TEXT("  <- PICKED") : TEXT(""));
        }

        Separator();

        // 최종 결정 한 줄 요약
        UE_LOG(Log_AI, Log, TEXT("  [Decision] Type: %-10s | Pattern: %-25s | Score: %.3f  <- PICKED"),
            *TypeName(Result.PickedType),
            *Result.PickedPatternID.ToString(),
            PickedScore);

        Separator();
    }

} // namespace CombatLog