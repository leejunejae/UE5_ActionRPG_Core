// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AI/CombatPatternData.h"  
#include "Navigation/PathFollowingComponent.h"
#include "BTTask_ExecutePattern_Alert.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBTTask_ExecutePattern_Alert : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
    UBTTask_ExecutePattern_Alert();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
    /** EQS 결과 콜백 */
    void OnEQSQueryFinished(TSharedPtr<FEnvQueryResult> QueryResult);

    /** MoveTo 완료 콜백 */
    UFUNCTION()
    void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

    /** 타임아웃 */
    void OnTimeout();

    /** 이동 설정 적용 */
    void ApplyMovementSettings(ACharacter* Character, const FAlertData& Data);

private:
    UBehaviorTreeComponent* OwnerCompRef = nullptr;

    int32 EQSRequestID = INDEX_NONE;
    FAIRequestID CurrentMoveRequestID;
    FTimerHandle TimeoutTimerHandle;

    // 현재 패턴 데이터 캐시 (콜백에서 AcceptanceRadius 등 참조용)
    FAlertData CachedAlertData;
};
