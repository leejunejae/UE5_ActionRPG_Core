// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/CombatDecisionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AI/CombatDecisionLog.h"
#include "Utils/CoreLog.h"

UCombatDecisionComponent::UCombatDecisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    Rng.Initialize(FMath::Rand()); // 필요하면 시드 노출해서 재현 가능
}

float UCombatDecisionComponent::GetLastUsedTime(FName PatternID) const
{
    if (const float* T = LastUsedTimeMap.Find(PatternID))
        return *T;
    return -100000.f;
}

void UCombatDecisionComponent::PushHistory(FName PatternID)
{
    RecentHistory.Insert(PatternID, 0);
    if (RecentHistory.Num() > RecentHistorySize)
        RecentHistory.SetNum(RecentHistorySize);
}

int32 UCombatDecisionComponent::WeightedPickIndex(const TArray<float>& Weights, FRandomStream& InRng)
{
    float Sum = 0.f;
    for (float W : Weights) Sum += FMath::Max(0.f, W);
    if (Sum <= KINDA_SMALL_NUMBER) return INDEX_NONE;

    const float Roll = InRng.FRandRange(0.f, Sum);
    float Acc = 0.f;

    for (int32 i = 0; i < Weights.Num(); ++i)
    {
        Acc += FMath::Max(0.f, Weights[i]);
        if (Roll <= Acc) return i;
    }
    return Weights.Num() - 1;
}

void UCombatDecisionComponent::NotifyPatternUsed(FName PatternID)
{
    if (PatternID.IsNone()) return;

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    LastUsedTimeMap.Add(PatternID, Now);
    PushHistory(PatternID);
}

float UCombatDecisionComponent::ComputeTypeScore(const TArray<FEvalPattern>& Patterns, int32 TopK, float CountBiasExp)
{
    if (Patterns.Num() == 0) return 0.f;

    TArray<float> Scores;
    Scores.Reserve(Patterns.Num());
    for (const auto& P : Patterns) Scores.Add(P.Score);
    Scores.Sort([](float A, float B) { return A > B; });

    const int32 K = FMath::Clamp(TopK, 1, Scores.Num());
    float SumTop = 0.f;
    for (int32 i = 0; i < K; ++i) SumTop += Scores[i];

    const float N = (float)Scores.Num();
    const float Denom = FMath::Pow(FMath::Max(1.f, N), CountBiasExp);

    return SumTop / Denom;
}

FCombatDecisionResult UCombatDecisionComponent::Decide(const FCombatContext& InCtx, const UDataTable* PatternTable, bool bDebugLog, const FString& DebugActorName)
{
    FCombatDecisionResult Out;

    if (!PatternTable)
    {
        CombatLog::NullTable(DebugActorName);
        return Out;
    }

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    FCombatContext Ctx = InCtx;

    // 1) DataTable rows 가져오기
    TArray<FCombatPattern*> Rows;
    PatternTable->GetAllRows(TEXT("CombatDecision"), Rows);

    // 2) 하드 통과 + 소프트 점수 계산
    TArray<FEvalPattern> Candidates;
    Candidates.Reserve(Rows.Num());

    if (bDebugLog) CombatLog::DecideHeader(DebugActorName);

    for (const FCombatPattern* Row : Rows)
    {
        if (!Row) continue;

        const float   LastUsed = GetLastUsedTime(Row->PatternID);
        const FString TypeStr = CombatLog::TypeName(Row->ActionType);

        if (!Row->IsAvailable(Ctx, LastUsed))
        {
            if (bDebugLog) CombatLog::PatternHardFail(Row->PatternID, TypeStr);
            continue;
        }

        float Score = Row->CalcScore(Ctx);

        // 반복 페널티
        if (RecentHistory.Num() > 0)
        {
            if (RecentHistory[0] == Row->PatternID)
                Score *= 0.25f;

            int32 Count = 0;
            for (const FName& H : RecentHistory)
                if (H == Row->PatternID) Count++;

            if (Count > 0)
                Score *= FMath::Pow(0.8f, (float)Count);
        }

        if (Score <= KINDA_SMALL_NUMBER)
        {
            if (bDebugLog) CombatLog::PatternTooLowScore(Row->PatternID, TypeStr, Score);
            continue;
        }

        if (bDebugLog) CombatLog::PatternPass(Row->PatternID, TypeStr, Score);

        Candidates.Add({ Row, Score });
        Out.PatternScores.Add(Row->PatternID, Score);
    }

    if (Candidates.Num() == 0)
    {
        CombatLog::NoValidCandidates();
        Out.PickedType = ECombatActionType::Chase;
        Out.PickedPatternID = NAME_None;
        if (bDebugLog) CombatLog::PrintResult(Out);
        return Out;
    }

    // 3) 타입별 그룹핑
    TMap<ECombatActionType, TArray<FEvalPattern>> ByType;
    for (const FEvalPattern& C : Candidates)
        ByType.FindOrAdd(C.Row->ActionType).Add(C);

    // 4) 타입 스코어 계산 + 타입 선택
    TArray<ECombatActionType> TypeList;
    TArray<float>             TypeWeights;

    for (auto& Pair : ByType)
    {
        const ECombatActionType     Type = Pair.Key;
        const TArray<FEvalPattern>& Pool = Pair.Value;

        float TypeScore = ComputeTypeScore(Pool, TypeTopK, TypeCountBiasExp);

        switch (Type)
        {
        case ECombatActionType::Attack:
            TypeScore *= FMath::Lerp(0.9f, 1.2f, Ctx.Aggressiveness);  break;
        case ECombatActionType::Recover:
            TypeScore *= FMath::Lerp(1.2f, 0.9f, Ctx.Aggressiveness);  break;
        case ECombatActionType::Defend:
            TypeScore *= FMath::Lerp(1.1f, 0.95f, Ctx.Aggressiveness); break;
        default: break;
        }

        Out.TypeScores.Add(Type, TypeScore);
        TypeList.Add(Type);
        TypeWeights.Add(TypeScore);
    }

    const int32 TypeIdx = WeightedPickIndex(TypeWeights, Rng);
    if (TypeIdx == INDEX_NONE)
    {
        CombatLog::TypePickFailed();
        Out.PickedType = ECombatActionType::Chase;
        Out.PickedPatternID = NAME_None;
        if (bDebugLog) CombatLog::PrintResult(Out);
        return Out;
    }

    Out.PickedType = TypeList[TypeIdx];

    // 5) 선택된 타입 내부에서 패턴 선택
    const TArray<FEvalPattern>& PickPool = ByType[Out.PickedType];

    TArray<float> PatternWeights;
    PatternWeights.Reserve(PickPool.Num());
    for (const FEvalPattern& P : PickPool)
        PatternWeights.Add(P.Score);

    const int32 PatternIdx = WeightedPickIndex(PatternWeights, Rng);
    if (PatternIdx != INDEX_NONE)
        Out.PickedPatternID = PickPool[PatternIdx].Row->PatternID;

    if (bDebugLog) CombatLog::PrintResult(Out);
    return Out;
}