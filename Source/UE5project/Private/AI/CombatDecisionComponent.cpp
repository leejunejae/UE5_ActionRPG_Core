// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/CombatDecisionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AI/CombatDecisionLog.h"
#include "AI/CombatPatternData.h"
#include "Utils/CoreLog.h"

UCombatDecisionComponent::UCombatDecisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatDecisionComponent::BeginPlay()
{
    Super::BeginPlay();

    // 시드 초기화 (외부에서 SeedRandomStream로 덮어쓸 수 있음)
    Rng.Initialize(static_cast<int32>(FDateTime::Now().GetTicks() & 0x7FFFFFFF));
}

// ===================================================================
// Public API
// ===================================================================

void UCombatDecisionComponent::SeedRandomStream(int32 Seed)
{
    Rng.Initialize(Seed);
}

void UCombatDecisionComponent::NotifyPatternUsed(FName PatternID)
{
    if (PatternID.IsNone()) return;

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    LastUsedTimeMap.Add(PatternID, Now);
    PushHistory(PatternID);
}

FCombatDecisionResult UCombatDecisionComponent::Decide(const FCombatContext& InCtx, const UDataTable* PatternTable, bool bDebugLog, const FString& DebugActorName)
{
    FCombatDecisionResult Out;

    if (!PatternTable)
    {
        CombatLog::NullTable(DebugActorName);
        return MakeFallbackResult(Out);
    }

    // 컨텍스트 검증/클램프
    FCombatContext Ctx = InCtx;
    ValidateContext(Ctx);

    // DataTable에서 직접 Row 가져오기
    TArray<FCombatPattern*> Rows;
    PatternTable->GetAllRows(TEXT("CombatDecision::Decide"), Rows);

    if (bDebugLog) CombatLog::DecideHeader(DebugActorName);

    // 1. 후보 평가
    TArray<FEvalPattern> Candidates;
    EvaluateCandidates(Ctx, Rows, Candidates, Out, bDebugLog);

    if (Candidates.Num() == 0)
    {
        CombatLog::NoValidCandidates();
        Out = MakeFallbackResult(Out);
        if (bDebugLog) CombatLog::PrintResult(Out);
        return Out;
    }

    // 2. 타입별 그룹핑 + 점수
    TMap<ECombatActionType, TArray<FEvalPattern>> ByType;
    TArray<ECombatActionType> TypeList;
    TArray<float> TypeWeights;
    GroupAndScoreTypes(Candidates, Ctx, ByType, TypeList, TypeWeights, Out);

    // 3. 타입 선택
    const ECombatActionType PickedType = PickType(TypeList, TypeWeights);
    if (PickedType == ECombatActionType::None)
    {
        CombatLog::TypePickFailed();
        Out = MakeFallbackResult(Out);
        if (bDebugLog) CombatLog::PrintResult(Out);
        return Out;
    }
    Out.PickedType = PickedType;

    // 4. 패턴 선택
    const TArray<FEvalPattern>& Pool = ByType[PickedType];
    Out.PickedPatternID = PickPattern(Pool);

    if (bDebugLog) CombatLog::PrintResult(Out);
    return Out;
}

// ===================================================================
// Decide Pipeline
// ===================================================================
void UCombatDecisionComponent::EvaluateCandidates(const FCombatContext& Ctx, const TArray<FCombatPattern*>& Rows, TArray<FEvalPattern>& OutCandidates, FCombatDecisionResult& InOutResult, bool bDebugLog) const
{
    OutCandidates.Reserve(Rows.Num());

    for (const FCombatPattern* Row : Rows)
    {
        if (!Row || Row->PatternID.IsNone()) continue;

        const float LastUsed = GetLastUsedTime(Row->PatternID);
        const FString TypeStr = CombatLog::TypeName(Row->ActionType);

        // 하드 조건
        if (!Row->IsAvailable(Ctx, LastUsed))
        {
            if (bDebugLog) CombatLog::PatternHardFail(Row->PatternID, TypeStr);
            continue;
        }

        // 소프트 점수
        float Score = Row->CalcScore(Ctx);

        // 반복 페널티
        Score *= CalcRepeatPenalty(Row->PatternID);

        if (Score <= KINDA_SMALL_NUMBER)
        {
            if (bDebugLog) CombatLog::PatternTooLowScore(Row->PatternID, TypeStr, Score);
            continue;
        }

        if (bDebugLog) CombatLog::PatternPass(Row->PatternID, TypeStr, Score);

        OutCandidates.Add({ Row, Score });
        InOutResult.PatternScores.Add(Row->PatternID, Score);
    }
}

void UCombatDecisionComponent::GroupAndScoreTypes(const TArray<FEvalPattern>& Candidates, const FCombatContext& Ctx, TMap<ECombatActionType, TArray<FEvalPattern>>& OutByType, TArray<ECombatActionType>& OutTypeList, TArray<float>& OutTypeWeights, FCombatDecisionResult& InOutResult) const
{
    // 그룹핑
    for (const FEvalPattern& C : Candidates)
    {
        OutByType.FindOrAdd(C.Row->ActionType).Add(C);
    }

    // 타입별 점수 계산
    OutTypeList.Reserve(OutByType.Num());
    OutTypeWeights.Reserve(OutByType.Num());

    for (const auto& Pair : OutByType)
    {
        const ECombatActionType Type = Pair.Key;
        const TArray<FEvalPattern>& Pool = Pair.Value;

        float TypeScore = ComputeTypeScore(Pool, TypeTopK, TypeCountBiasExp);

        // Aggressiveness 기반 타입 보정 (한 곳에서만 적용)
        TypeScore *= GetAggressivenessMultiplier(Type, Ctx.Aggressiveness);

        InOutResult.TypeScores.Add(Type, TypeScore);
        OutTypeList.Add(Type);
        OutTypeWeights.Add(TypeScore);
    }
}

ECombatActionType UCombatDecisionComponent::PickType( const TArray<ECombatActionType>& TypeList, const TArray<float>& TypeWeights)
{
    const int32 Idx = WeightedPickIndex(TypeWeights, Rng);
    return (Idx == INDEX_NONE) ? ECombatActionType::None : TypeList[Idx];
}

FName UCombatDecisionComponent::PickPattern(const TArray<FEvalPattern>& Pool)
{
    if (Pool.Num() == 0) return NAME_None;

    TArray<float> Weights;
    Weights.Reserve(Pool.Num());
    for (const FEvalPattern& P : Pool)
    {
        Weights.Add(P.Score);
    }

    const int32 Idx = WeightedPickIndex(Weights, Rng);
    return (Idx == INDEX_NONE) ? NAME_None : Pool[Idx].Row->PatternID;
}

FCombatDecisionResult UCombatDecisionComponent::MakeFallbackResult(FCombatDecisionResult InOutResult) const
{
    InOutResult.PickedType = FallbackAction;
    InOutResult.PickedPatternID = FallbackPatternID;
    return InOutResult;
}

// ===================================================================
// Helpers
// ===================================================================
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

void UCombatDecisionComponent::ValidateContext(FCombatContext& Ctx) const
{
#if !UE_BUILD_SHIPPING
    if (Ctx.HPPercent < 0.f || Ctx.HPPercent > 1.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDecision] HPPercent out of range: %.2f"), Ctx.HPPercent);
    }
    if (Ctx.Aggressiveness < 0.f || Ctx.Aggressiveness > 1.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDecision] Aggressiveness out of range: %.2f"), Ctx.Aggressiveness);
    }
    if (Ctx.Phase < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDecision] Phase < 1: %d"), Ctx.Phase);
    }
    if (Ctx.DistanceToTarget < 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDecision] DistanceToTarget < 0: %.2f"), Ctx.DistanceToTarget);
    }
#endif

    Ctx.HPPercent = FMath::Clamp(Ctx.HPPercent, 0.f, 1.f);
    Ctx.Aggressiveness = FMath::Clamp(Ctx.Aggressiveness, 0.f, 1.f);
    Ctx.Phase = FMath::Max(1, Ctx.Phase);
    Ctx.DistanceToTarget = FMath::Max(0.f, Ctx.DistanceToTarget);
    Ctx.AbsDeltaYawDeg = FMath::Abs(Ctx.AbsDeltaYawDeg);
}

float UCombatDecisionComponent::CalcRepeatPenalty(FName PatternID) const
{
    if (RecentHistory.Num() == 0) return 1.f;

    float Penalty = 1.f;

    // 직전 패턴: 강한 페널티 (별도 적용)
    if (RecentHistory[0] == PatternID)
    {
        Penalty *= ImmediateRepeatPenalty;
    }

    // 빈도 페널티 (직전은 제외하고 카운트)
    int32 Count = 0;
    for (int32 i = 1; i < RecentHistory.Num(); ++i)
    {
        if (RecentHistory[i] == PatternID)
        {
            ++Count;
        }
    }

    if (Count > 0)
    {
        Penalty *= FMath::Pow(HistoryDecayBase, static_cast<float>(Count));
    }

    return Penalty;
}

float UCombatDecisionComponent::GetAggressivenessMultiplier(ECombatActionType Type, float Aggressiveness) const
{
    const float A = FMath::Clamp(Aggressiveness, 0.f, 1.f);

    switch (Type)
    {
    case ECombatActionType::Attack:
        return FMath::Lerp(AttackAggrRange.X, AttackAggrRange.Y, A);

    case ECombatActionType::Recover:
        return FMath::Lerp(RecoverAggrRange.X, RecoverAggrRange.Y, A);

    case ECombatActionType::Defend:
        return FMath::Lerp(DefendAggrRange.X, DefendAggrRange.Y, A);

    case ECombatActionType::Evasion:
        return FMath::Lerp(EvasionAggrRange.X, EvasionAggrRange.Y, A);

    default:
        return 1.f;
    }
}

// ===================================================================
// Static Helpers
// ===================================================================
int32 UCombatDecisionComponent::WeightedPickIndex(const TArray<float>& Weights, FRandomStream& InRng)
{
    if (Weights.Num() == 0) return INDEX_NONE;

    // 한 번에 정제 + 합산
    TArray<float> Clamped;
    Clamped.Reserve(Weights.Num());

    float Sum = 0.f;
    for (float W : Weights)
    {
        const float C = FMath::Max(0.f, W);
        Clamped.Add(C);
        Sum += C;
    }

    if (Sum <= KINDA_SMALL_NUMBER) return INDEX_NONE;

    const float Roll = InRng.FRandRange(0.f, Sum);
    float Acc = 0.f;
    for (int32 i = 0; i < Clamped.Num(); ++i)
    {
        Acc += Clamped[i];
        if (Roll <= Acc) return i;
    }

    return Clamped.Num() - 1;
}

float UCombatDecisionComponent::ComputeTypeScore(const TArray<FEvalPattern>& Patterns, int32 TopK, float CountBiasExp)
{
    if (Patterns.Num() == 0) return 0.f;

    TArray<float> Scores;
    Scores.Reserve(Patterns.Num());
    for (const FEvalPattern& P : Patterns)
    {
        Scores.Add(P.Score);
    }
    Scores.Sort([](float A, float B) { return A > B; });

    const int32 K = FMath::Clamp(TopK, 1, Scores.Num());
    float SumTop = 0.f;
    for (int32 i = 0; i < K; ++i)
    {
        SumTop += Scores[i];
    }

    const float N = static_cast<float>(Scores.Num());
    const float Denom = FMath::Pow(FMath::Max(1.f, N), CountBiasExp);

    return SumTop / Denom;
}