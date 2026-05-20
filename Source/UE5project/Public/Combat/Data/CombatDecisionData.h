// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "Utils/CoreLog.h"
#include "CombatDecisionData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ECombatActionType : uint8
{
    None    UMETA(DisplayName = "None"),
    Attack  UMETA(DisplayName = "Attack"),
    Defend   UMETA(DisplayName = "Defend"),
    Evasion   UMETA(DisplayName = "Evasion"),
    Reposition   UMETA(DisplayName = "Reposition"),
    Alert   UMETA(DisplayName = "Alert"),
    Recover   UMETA(DisplayName = "Recover"),
    Flee UMETA(DisplayName = "Flee"),
};

/**
 * 전투 의사결정에 필요한 런타임 컨텍스트
 */
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
 *  Pattern Evaluation Criteria for HardFailed / SoftScore
 * ============================================================ */
UENUM(BlueprintType)
enum class EScoreMode : uint8
{
    /** 조건 범위 안이면 1.0, 밖이면 0 (현재 동작) */
    Binary              UMETA(DisplayName = "Binary"),

    /** Ideal 지점에 가까울수록 1, 경계로 갈수록 0 — Attack에 적합 */
    PeakAtIdeal         UMETA(DisplayName = "Peak at Ideal"),

    /** 값이 클수록 점수 ↑ (Min~Max 사이를 0~1로 보간) — Chase에 적합 */
    HigherIsBetter      UMETA(DisplayName = "Higher Is Better"),

    /** 값이 작을수록 점수 ↑ — Retreat이 가까운 거리에서 더 선호되도록 할 때 */
    LowerIsBetter       UMETA(DisplayName = "Lower Is Better"),
};

USTRUCT(BlueprintType)
struct FRangeCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseRange"))
    float MinRange = 0.f;

    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseRange"))
    float MaxRange = 99999.f;

    /** Range 점수화 방식 */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseRange"))
    EScoreMode ScoreMode = EScoreMode::Binary;

    /** PeakAtIdeal 모드일 때 사용 */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseRange && ScoreMode == EScoreMode::PeakAtIdeal"))
    float IdealRange = 200.f;

    /** 이 조건의 점수가 최종 점수에 미치는 영향력 (가중치) */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseRange", ClampMin = "0.0"))
    float Weight = 1.0f;

    /** Min/Max 밖이면 하드 실패인가, 그냥 점수 0인가 */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseRange"))
    bool bHardFail = true;

    bool PassesHardCheck(float Value) const;
    float CalcScore(float Value) const;
};

USTRUCT(BlueprintType)
struct FHPCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinHPPercent = 0.f;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxHPPercent = 1.f;

    UPROPERTY(EditAnywhere)
    EScoreMode ScoreMode = EScoreMode::Binary;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IdealHPPercent = 0.5f;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere)
    bool bHardFail = false; // HP는 보통 점수만 영향 (예외: Recover는 HP 낮을 때만)

    bool PassesHardCheck(float Value) const;
    float CalcScore(float Value) const;
};

USTRUCT(BlueprintType)
struct FAngleCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float AngleGoodDeg = 30.f; // 이 안이면 점수 1

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float AngleBadDeg = 90.f;  // 이 밖이면 점수 0

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere)
    bool bHardFail = false; // 일반적으로 부드러운 감점

    bool PassesHardCheck(float Value) const;
    float CalcScore(float Value) const;
};

USTRUCT(BlueprintType)
struct FPatternCondition
{
    GENERATED_BODY()

    // ---------- Range ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range")
    bool bUseRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range", meta = (EditCondition = "bUseRange", EditConditionHides))
    FRangeCondition RangeCondition;

    // ---------- Angle ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Angle")
    bool bUseAngle = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Angle", meta = (EditCondition = "bUseAngle", EditConditionHides))
    FAngleCondition AngleCondition;

    // ---------- HP ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HP")
    bool bUseHPRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HP", meta = (EditCondition = "bUseHPRange", EditConditionHides))
    FHPCondition HPCondition;

    // ---------- Phase ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
    bool bUsePhase = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase", meta = (EditCondition = "bUsePhase", EditConditionHides, ClampMin = "1"))
    int32 RequiredPhase = 1;

    // ---------- Status Flags ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    bool bRequirePoiseBroken = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    bool bRequireStanceBroken = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    bool bRequireRangedThreat = false;

    // ---------- Cooldown ----------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (ClampMin = "0.0"))
    float Cooldown = 0.f;

    bool IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const;
    float CalcScore(const FCombatContext& Ctx) const;
};

/* ============================================================
 *  Combat Pattern - Reposition
 * ============================================================ */

UENUM(BlueprintType)
enum class ERepositionDirection : uint8
{
    /** 대상 방향으로 접근 — Actor 추적 이동 */
    TowardTarget        UMETA(DisplayName = "Toward Target"),
    /** 대상 반대 방향으로 이동 (Retreat / BackStep) */
    AwayFromTarget      UMETA(DisplayName = "Away From Target"),
    /** 대상 기준 좌측으로 이동 */
    SideLeft            UMETA(DisplayName = "Side Left"),
    /** 대상 기준 우측으로 이동 */
    SideRight           UMETA(DisplayName = "Side Right"),
    /** 좌/우 중 무작위 선택 */
    RandomSide          UMETA(DisplayName = "Random Side"),
};

USTRUCT(BlueprintType)
struct FRepositionData
{
    GENERATED_BODY()

    // 이 패턴이 어떤 의도인가
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
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float AcceptanceRadius = 20.f;

    /** 이동 시 적용할 이동 속도 (CharacterMovement.MaxWalkSpeed) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float MovementSpeed = 300.f;

    /** 이동 최대 지속 시간 (도달 못해도 종료) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.1"))
    float MaxDuration = 3.0f;
};

/* ============================================================
 *  Combat Pattern Data
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

    UPROPERTY(EditAnywhere, meta = (EditCondition = "ActionType == ECombatActionType::Reposition", EditConditionHides))
        FRepositionData RepositionData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float BaseScore = 1.f; // 기본 가중치

        // 런타임 관리용

public:

    bool IsAvailable(const FCombatContext& Ctx, float LastUsedTime) const
    {
        return Conditions.IsAvailable(Ctx, LastUsedTime);
    }
    float CalcScore(const FCombatContext& Ctx) const;

private:
    float CalcSituationalMultiplier(const FCombatContext& Ctx) const;
};

USTRUCT(BlueprintType)
struct FCombatDecisionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) ECombatActionType PickedType = ECombatActionType::None;
    UPROPERTY(BlueprintReadOnly) FName PickedPatternID = NAME_None;

    // 디버그용
    UPROPERTY(BlueprintReadOnly) TMap<ECombatActionType, float> TypeScores;
    UPROPERTY(BlueprintReadOnly) TMap<FName, float> PatternScores;
};

struct FEvalPattern
{
    const FCombatPattern* Row = nullptr;
    float Score = 0.f;
};

UCLASS()
class UE5PROJECT_API UCombatDecisionData : public UObject
{
	GENERATED_BODY()
	
};
