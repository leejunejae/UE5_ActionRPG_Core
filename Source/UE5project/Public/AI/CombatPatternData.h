// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "Combat/Data/CombatDecisionData.h"
#include "AI/CombatPatternCondition.h"
#include "CombatPatternData.generated.h"

 /* ============================================================
  *  Combat Pattern - Reposition
  * ============================================================ */

UENUM(BlueprintType)
enum class ERepositionDirection : uint8
{
    /** 대상 방향으로 접근 — Actor 추적 이동 */
    TowardTarget    UMETA(DisplayName = "Toward Target"),
    /** 대상 반대 방향으로 이동 (Retreat / BackStep) */
    AwayFromTarget  UMETA(DisplayName = "Away From Target"),
    /** 대상 기준 좌측으로 이동 */
    SideLeft        UMETA(DisplayName = "Side Left"),
    /** 대상 기준 우측으로 이동 */
    SideRight       UMETA(DisplayName = "Side Right"),
    /** 좌/우 중 무작위 선택 */
    RandomSide      UMETA(DisplayName = "Random Side"),
};

USTRUCT(BlueprintType)
struct FRepositionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    ERepositionDirection DirectionMode = ERepositionDirection::AwayFromTarget;

    /**
    * TowardTarget: 대상과 최종 유지할 거리
    * AwayFromTarget: 대상으로부터 도달할 거리
    * Side*: 대상으로부터 유지할 거리 (측면 위치)
    */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "10.0"))
    float TargetDistance = 300.f;

    /** 도착 판정 허용 반경 */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "DirectionMode != ERepositionDirection::TowardTarget", ClampMin = "0.0"))
    float AcceptanceRadius = 20.f;

    /** 이동 시 적용할 이동 속도 (CharacterMovement.MaxWalkSpeed) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float MovementSpeed = 300.f;

    /** 이동 최대 지속 시간 (도달 못해도 종료) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.1"))
    float MaxDuration = 3.0f;
};

/* ============================================================
 *  Combat Pattern - Alert
 * ============================================================ */

USTRUCT(BlueprintType)
struct FAlertData
{
    GENERATED_BODY()

    /** 위치 후보 평가에 사용할 EQS Query */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alert")
    TObjectPtr<UEnvQuery> EQSQuery = nullptr;

    /** Alert 이동 속도 (보통 걷기 속도) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alert", meta = (ClampMin = "0.0"))
    float MovementSpeed = 250.f;

    /** 도착 허용 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alert", meta = (ClampMin = "0.0"))
    float AcceptanceRadius = 50.f;

    /** 안전장치: 이 시간 안에 도착 못 하면 강제 종료 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alert", meta = (ClampMin = "0.5"))
    float MaxDuration = 4.0f;
};

/* ============================================================
 *  Combat Pattern - DataTable Row
 * ============================================================ */

USTRUCT(BlueprintType)
struct FCombatPattern : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName PatternID; // 공격 이름, 몽타주 키 등

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECombatActionType ActionType = ECombatActionType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FPatternCondition Conditions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "ActionType == ECombatActionType::Reposition", EditConditionHides))
    FRepositionData RepositionData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "ActionType == ECombatActionType::Alert", EditConditionHides))
    FAlertData AlertData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float BaseScore = 1.f; // 기본 가중치

public:
    bool IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const
    {
        return Conditions.IsAvailable(Ctx, LastUsedTime);
    }

    float CalcScore(const FCombatContext& Ctx) const;

private:
    float CalcSituationalMultiplier(const FCombatContext& Ctx) const;
};