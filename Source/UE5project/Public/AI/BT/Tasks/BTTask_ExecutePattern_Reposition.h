// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Combat/Data/CombatDecisionData.h"
#include "Navigation/PathFollowingComponent.h"
#include "BTTask_ExecutePattern_Reposition.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBTTask_ExecutePattern_Reposition : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_ExecutePattern_Reposition();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
    // ============================================================
    // 정적 헬퍼: 위치 계산 (상태 없음, 순수 함수)
    // ============================================================

    /** DirectionMode에 따른 단위 방향 벡터 반환 (대상 → 자신 기준 정규화 + 회전) */
    static FVector ResolveDirection(ERepositionDirection Intent, const FVector& SelfLoc, const FVector& TargetLoc);

    /** 고정 위치 계산 (TowardTarget이 아닌 경우) + NavMesh 투영 */
    static bool CalculateFixedLocation(UWorld* World, const FRepositionData& Data, const FVector& SelfLoc, const FVector& TargetLoc, FVector& OutLocation);

    /** NavMesh로 좌표 투영 */
    static bool ProjectToNavMesh(
        UWorld* World,
        const FVector& InLocation,
        FVector& OutLocation);

    // ============================================================
    // 이동 설정 적용/복귀 (인스턴스 상태 필요)
    // ============================================================

    void ApplyMovementSettings(class ACharacter* Character, const FRepositionData& Data);
    void RestoreMovementSettings(class ACharacter* Char);

    // ============================================================
    // 콜백
    // ============================================================

    UFUNCTION()
    void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

    void OnTimeout();

private:
    /** MoveTo 요청 ID — 다른 MoveTo 요청과 구분용 */
    FAIRequestID CurrentMoveRequestID;

    /** 타임아웃 핸들 */
    FTimerHandle TimeoutTimerHandle;

    /** OwnerComp 참조 (콜백에서 사용) */
    UPROPERTY()
    TObjectPtr<UBehaviorTreeComponent> OwnerCompRef = nullptr;
};
