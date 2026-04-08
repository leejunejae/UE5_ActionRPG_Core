// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/CombatDecisionComponent.h"
#include "Kismet/KismetMathLibrary.h"
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

FCombatDecisionResult UCombatDecisionComponent::Decide(const FCombatContext& InCtx, const UDataTable* PatternTable, bool bDebugLog)
{
    FCombatDecisionResult Out;

    if (!PatternTable)
    {
        if (bDebugLog)
        {
            UE_LOG(Log_AI, Warning, TEXT("[CombatDecision] PatternTable is null (%s)"),
                *GetOwner()->GetActorNameOrLabel());
        }
        return Out;
    }

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    // 컨텍스트 복사 + 히스토리 주입
    FCombatContext Ctx = InCtx;

    // 1) DataTable rows 가져오기
    // GetAllRows는 매번 호출하면 비용이 있으니, 실제 프로젝트에선 BeginPlay에서 캐싱 추천
    TArray<FCombatPattern*> Rows;
    PatternTable->GetAllRows(TEXT("CombatDecision"), Rows);

    // 2) 하드 통과 + 소프트 점수 계산
    TArray<FEvalPattern> Candidates;
    Candidates.Reserve(Rows.Num());

    UE_LOG(Log_AI, Log, TEXT("[CombatDecisionData] Evaluate Pattern"));

    for (const FCombatPattern* Row : Rows)
    {
        if (!Row) continue;

        const float LastUsed = GetLastUsedTime(Row->PatternID);

        UE_LOG(Log_AI, Log, TEXT("Pattern : %s - Type : %s \n"), 
            *Row->PatternID.ToString(),
            *StaticEnum<ECombatActionType>()->GetNameStringByValue((int64)Row->ActionType)
            );

        UE_LOG(Log_AI, Log, TEXT("  [Hard Condition]"));

        if (!Row->IsAvailable(Ctx, LastUsed))
            continue;

        float Score = Row->CalcScore(Ctx);

        // ---- 반복 페널티(컴포넌트에서 처리: Row는 순수하게 유지하고 싶을 때 여기에 둠) ----
        if (RecentHistory.Num() > 0)
        {
            const FName Last = RecentHistory[0];
            if (Last == Row->PatternID)
                Score *= 0.25f;

            int32 Count = 0;
            for (const FName& H : RecentHistory)
                if (H == Row->PatternID) Count++;

            if (Count > 0)
                Score *= FMath::Pow(0.8f, (float)Count);
        }

        UE_LOG(Log_AI, Log, TEXT("  [Soft Score]\n Pattern Score = %f"), Score);

        if (Score <= KINDA_SMALL_NUMBER)
            continue;

        Candidates.Add({ Row, Score });

        // 디버그용: 패턴 점수 저장
        Out.PatternScores.Add(Row->PatternID, Score);
    }

    if (Candidates.Num() == 0)
    {
        // fallback
        Out.PickedType = ECombatActionType::Chase;
        Out.PickedPatternID = NAME_None;
        if (bDebugLog) DebugPrint(Out);
        return Out;
    }
    
    // 3) 타입별 그룹핑
    TMap<ECombatActionType, TArray<FEvalPattern>> ByType;
    for (const FEvalPattern& C : Candidates)
    {
        ByType.FindOrAdd(C.Row->ActionType).Add(C);
    }

    // 4) 타입 스코어 계산 + 타입 선택
    TArray<ECombatActionType> TypeList;
    TArray<float> TypeWeights;

    UE_LOG(Log_AI, Log, TEXT("Evaluate TypeScore\n"));

    for (auto& Pair : ByType)
    {
        const ECombatActionType Type = Pair.Key;
        const TArray<FEvalPattern>& Pool = Pair.Value;

        float TypeScore = ComputeTypeScore(Pool, TypeTopK, TypeCountBiasExp);

        // 타입 전역 성향 보정(원하면 더 추가)
        switch (Type)
        {
        case ECombatActionType::Attack:
            TypeScore *= FMath::Lerp(0.9f, 1.2f, Ctx.Aggressiveness);
            break;
        case ECombatActionType::Recover:
            TypeScore *= FMath::Lerp(1.2f, 0.9f, Ctx.Aggressiveness);
            break;
        case ECombatActionType::Defend:
            TypeScore *= FMath::Lerp(1.1f, 0.95f, Ctx.Aggressiveness);
            break;
        default:
            break;
        }

        Out.TypeScores.Add(Type, TypeScore);
        TypeList.Add(Type);
        TypeWeights.Add(TypeScore);

        UE_LOG(Log_AI, Log, TEXT("Type : %s - Score : %f \n"),
            *StaticEnum<ECombatActionType>()->GetNameStringByValue((int64)Type),
            TypeScore);
    }

    const int32 TypeIdx = WeightedPickIndex(TypeWeights, Rng);
    if (TypeIdx == INDEX_NONE)
    {
        Out.PickedType = ECombatActionType::Chase;
        Out.PickedPatternID = NAME_None;
        if (bDebugLog) DebugPrint(Out);
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
    {
        Out.PickedPatternID = PickPool[PatternIdx].Row->PatternID;
    }

    if (bDebugLog) DebugPrint(Out);
    return Out;
}

void UCombatDecisionComponent::DebugPrint(const FCombatDecisionResult& Result) const
{
    FString TypeStr;
    for (auto& Pair : Result.TypeScores)
    {
        TypeStr += FString::Printf(TEXT("  Type=%d Score=%.3f\n"), (int32)Pair.Key, Pair.Value);
    }

    FString PatStr;
    // 너무 길어질 수 있으니 상위 몇 개만 보고 싶으면 정렬해서 출력해도 됨
    for (auto& Pair : Result.PatternScores)
    {
        PatStr += FString::Printf(TEXT("  Pattern=%s Score=%.3f\n"), *Pair.Key.ToString(), Pair.Value);
    }

    UE_LOG(Log_AI, Log, TEXT("[CombatDecision][%s]\nPickedType=%s PickedPattern=%s\n--TypeScores--\n%s--PatternScores--\n%s"),
        *GetOwner()->GetActorNameOrLabel(),
        *StaticEnum<ECombatActionType>()->GetNameStringByValue((int64)Result.PickedType),
        *Result.PickedPatternID.ToString(),
        *TypeStr,
        *PatStr);
}

