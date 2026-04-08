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


    /** 패턴 테이블 (RowName 상관없이 Row.PatternID를 사용해도 됨) */
   // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision")
        //UDataTable* PatternTable = nullptr;

    /** 최근 N개 패턴 히스토리로 반복 페널티 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning")
        int32 RecentHistorySize = 6;

    /** 타입 스코어 계산: TopK 합 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning")
        int32 TypeTopK = 2;

    /** 타입 스코어 정규화: / pow(n, CountBiasExp). 0.5면 /sqrt(n) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDecision|Tuning")
        float TypeCountBiasExp = 0.5f;

    /** (예시) 장비 상태 - 실제 프로젝트에선 캐릭터/컴포넌트에서 가져오면 됨 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Runtime")
        bool bHasShield = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatDecision|Runtime")
        bool bHasLightArmor = false;

    /** 외부에서 호출: 컨텍스트를 주면 최종 패턴을 선택 */
    UFUNCTION(BlueprintCallable, Category = "CombatDecision")
        FCombatDecisionResult Decide(const FCombatContext& InCtx, const UDataTable* PatternTable, bool bDebugLog = false);

    /** 선택된 패턴을 실제로 사용했다고 기록(쿨다운/히스토리 갱신) */
    UFUNCTION(BlueprintCallable, Category = "CombatDecision")
        void NotifyPatternUsed(FName PatternID);

private:
    /** AI 개체별 런타임 상태 */
    TMap<FName, float> LastUsedTimeMap;
    TArray<FName> RecentHistory;

    /** 랜덤(재현성 원하면 시드 노출해도 됨) */
    FRandomStream Rng;

private:
    static int32 WeightedPickIndex(const TArray<float>& Weights, FRandomStream& InRng);

    float GetLastUsedTime(FName PatternID) const;
    void PushHistory(FName PatternID);

    void DebugPrint(const FCombatDecisionResult& Result) const;

    float ComputeTypeScore(const TArray<FEvalPattern>& Patterns, int32 TopK, float CountBiasExp);
};
