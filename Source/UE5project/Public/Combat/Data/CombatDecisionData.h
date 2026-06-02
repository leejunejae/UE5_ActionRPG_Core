// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatDecisionData.generated.h"

/* ============================================================
 *  Combat Action Type
 * ============================================================ */

UENUM(BlueprintType)
enum class ECombatActionType : uint8
{
    None        UMETA(DisplayName = "None"),
    Attack      UMETA(DisplayName = "Attack"),
    Defend      UMETA(DisplayName = "Defend"),
    Evasion     UMETA(DisplayName = "Evasion"),
    Reposition  UMETA(DisplayName = "Reposition"),
    Alert       UMETA(DisplayName = "Alert"),
    Recover     UMETA(DisplayName = "Recover"),
    Flee        UMETA(DisplayName = "Flee"),
};

/* ============================================================
 *  Runtime Combat Context (의사결정 입력)
 * ============================================================ */

USTRUCT(BlueprintType)
struct FCombatContext
{
    GENERATED_BODY()

public:
    /** 타겟까지의 거리 (cm) */
    UPROPERTY(BlueprintReadWrite, Category = "Spatial")
    float DistanceToTarget = 0.f;

    /** 타겟 방향과의 절대 Yaw 차이 (도) */
    UPROPERTY(BlueprintReadWrite, Category = "Spatial")
    float AbsDeltaYawDeg = 0.f;

    /** 시야 확보 여부 */
    UPROPERTY(BlueprintReadWrite, Category = "Spatial")
    bool bHasLOS = true;

    /** 자기 HP 비율 0~1 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    float HPPercent = 1.f;

    /** 현재 페이즈 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    int32 Phase = 1;

    /** 자기 경직 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    bool bPoiseBroken = false;

    /** 자기 가드 붕괴 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Self State")
    bool bStanceBroken = false;

    /** 공격 성향 0~1 (캐릭터 personality에서 주입) */
    UPROPERTY(BlueprintReadWrite, Category = "Personality")
    float Aggressiveness = 0.5f;

    /** 타겟이 공격 후딜 중인가 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetInRecovery = false;

    /** 타겟이 가드 중인가 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetGuarding = false;

    /** 타겟이 공격 모션 중인가 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetAttacking = false;

    /** 타겟 경직 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetPoiseBroken = false;

    /** 타겟 가드 붕괴 상태 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bTargetStanceBroken = false;

    /** 원거리 위협 존재 여부 */
    UPROPERTY(BlueprintReadWrite, Category = "Target State")
    bool bRangedThreat = false;

    /** 현재 시간 (월드 시간) */
    UPROPERTY(BlueprintReadWrite, Category = "Time")
    float CurrentTime = 0.f;
};

/* ============================================================
 *  Decision Result (의사결정 출력)
 * ============================================================ */

USTRUCT(BlueprintType)
struct FCombatDecisionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ECombatActionType PickedType = ECombatActionType::None;

    UPROPERTY(BlueprintReadOnly)
    FName PickedPatternID = NAME_None;

    // 디버그용
    UPROPERTY(BlueprintReadOnly)
    TMap<ECombatActionType, float> TypeScores;

    UPROPERTY(BlueprintReadOnly)
    TMap<FName, float> PatternScores;
};

/* ============================================================
 *  Internal — DecisionComponent의 후보 정렬용
 * ============================================================ */

 // FCombatPattern을 가리키므로 forward declare
struct FCombatPattern;

struct FEvalPattern
{
    const FCombatPattern* Row = nullptr;
    float Score = 0.f;
};