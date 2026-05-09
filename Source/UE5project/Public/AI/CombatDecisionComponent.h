// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Data/CombatDecisionData.h"
#include "CombatDecisionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UCombatDecisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatDecisionComponent();

    // ============================================================
    // Tuning - Repeat Penalty
    // ============================================================
    /** 직전에 사용한 패턴에 대한 페널티 배수 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning|Repeat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ImmediateRepeatPenalty = 0.25f;

    /** 빈도 기반 페널티 베이스 (count 누적 시 base^count 적용) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning|Repeat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HistoryDecayBase = 0.8f;

    /** 최근 N개 패턴 히스토리로 반복 페널티 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning|Repeat", meta = (ClampMin = "1"))
    int32 RecentHistorySize = 6;

    // ============================================================
    // Tuning - Type Score
    // ============================================================
    /** 타입 스코어 계산: TopK 합 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning|TypeScore", meta = (ClampMin = "1"))
    int32 TypeTopK = 2;

    /** 타입 스코어 정규화: / pow(n, CountBiasExp). 0.5면 /sqrt(n) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning|TypeScore", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float TypeCountBiasExp = 0.5f;

    // ============================================================
    // Tuning - Aggressiveness 보정 (X=0일때, Y=1일때)
    // ============================================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Tuning|Aggressiveness")
    FVector2D AttackAggrRange = FVector2D(0.9f, 1.2f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Tuning|Aggressiveness")
    FVector2D RecoverAggrRange = FVector2D(1.2f, 0.9f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Tuning|Aggressiveness")
    FVector2D DefendAggrRange = FVector2D(1.1f, 0.95f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Tuning|Aggressiveness")
    FVector2D EvasionAggrRange = FVector2D(1.0f, 0.95f);

    // ============================================================
    // Tuning - Fallback
    // ============================================================
    /** 후보가 하나도 없을 때 선택할 기본 행동 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Tuning|Fallback")
    ECombatActionType FallbackAction = ECombatActionType::Reposition;

    /** 후보가 하나도 없을 때 선택할 기본 패턴 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Tuning|Fallback")
    FName FallbackPatternID = NAME_None;

    /** (예시) 장비 상태 - 실제 프로젝트에선 캐릭터/컴포넌트에서 가져오면 됨 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Runtime")
        bool bHasShield = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Runtime")
        bool bHasLightArmor = false;

public:
    // ============================================================
    // Public API
    // ============================================================
    /**
        * 외부에서 호출: 컨텍스트와 패턴 테이블을 받아 최종 패턴을 선택
        * @param InCtx           평가 컨텍스트 (내부에서 검증/클램프됨)
        * @param PatternTable    선택지 풀
        * @param bDebugLog       디버그 로깅 여부
        * @param DebugActorName  디버그 로그용 이름
        */
    UFUNCTION(BlueprintCallable, Category = "CombatDecision")
    FCombatDecisionResult Decide(const FCombatContext& InCtx, const UDataTable* PatternTable, bool bDebugLog = true, const FString& DebugActorName = TEXT(""));

    /** 선택된 패턴을 사용했다고 기록 (쿨다운/히스토리 갱신) */
    UFUNCTION(BlueprintCallable, Category = "CombatDecision")
    void NotifyPatternUsed(FName PatternID);

    /** 외부에서 시드 지정 (디터미니즘 필요 시) */
    UFUNCTION(BlueprintCallable, Category = "CombatDecision")
    void SeedRandomStream(int32 Seed);

protected:
    virtual void BeginPlay() override;

private:
// ============================================================
// Runtime State
// ============================================================
    /** 패턴별 마지막 사용 시각 */
    TMap<FName, float> LastUsedTimeMap;

    /** 최근 사용 패턴 이력 (앞쪽이 가장 최근) */
    TArray<FName> RecentHistory;

    /** 의사결정용 난수 스트림 */
    FRandomStream Rng;

private:
// ============================================================
// Helpers
// ============================================================
    float GetLastUsedTime(FName PatternID) const;
    void  PushHistory(FName PatternID);

    /** Context 값을 안전 범위로 클램프 + 디버그 경고 */
    void  ValidateContext(FCombatContext& Ctx) const;

    /** 직전/빈도 페널티를 합쳐 반환 */
    float CalcRepeatPenalty(FName PatternID) const;

    /** Aggressiveness 기반 타입 보정 배수 반환 */
    float GetAggressivenessMultiplier(ECombatActionType Type, float Aggressiveness) const;

// ============================================================
// Decide Pipeline
// ============================================================
/** 1단계: 후보 평가 (하드 통과 + 소프트 점수 + 페널티) */
    void EvaluateCandidates(
        const FCombatContext& Ctx,
        const TArray<FCombatPattern*>& Rows,
        TArray<FEvalPattern>& OutCandidates,
        FCombatDecisionResult& InOutResult,
        bool bDebugLog) const;

    /** 2단계: 타입별 그룹핑 + 타입 점수 계산 */
    void GroupAndScoreTypes(
        const TArray<FEvalPattern>& Candidates,
        const FCombatContext& Ctx,
        TMap<ECombatActionType, TArray<FEvalPattern>>& OutByType,
        TArray<ECombatActionType>& OutTypeList,
        TArray<float>& OutTypeWeights,
        FCombatDecisionResult& InOutResult) const;

    /** 3단계: 타입 선택 */
    ECombatActionType PickType(
        const TArray<ECombatActionType>& TypeList,
        const TArray<float>& TypeWeights);

    /** 4단계: 패턴 선택 */
    FName PickPattern(const TArray<FEvalPattern>& Pool);

    /** Fallback 결과 생성 */
    FCombatDecisionResult MakeFallbackResult(FCombatDecisionResult InOutResult) const;

// ============================================================
// Static Helpers
// ============================================================
    static int32 WeightedPickIndex(const TArray<float>& Weights, FRandomStream& InRng);
    static float ComputeTypeScore(const TArray<FEvalPattern>& Patterns, int32 TopK, float CountBiasExp);
};
