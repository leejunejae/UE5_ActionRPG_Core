// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/Data/CombatDecisionData.h"
#include "EnemyBaseAIController.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EAIBehaviorState : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Alert UMETA(DisplayName = "Alert"),
	Combat UMETA(DisplayName = "Combat"),
	Flee UMETA(DisplayName = "Flee"),
};

UENUM(BlueprintType)
enum class EThreatSense : uint8
{
    None,
    Sight,
    Hearing,
    Damage,
};

USTRUCT(BlueprintType)
struct FThreatEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        TWeakObjectPtr<AActor> Target;
    /** 지금(가장 최근 이벤트 기준) 감지에 성공한 상태인가 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        bool bCurrentlySensed = false;

    /** Perception이 이 대상을 완전히 '잊었다'(MaxAge 만료) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        bool bForgottened = false;

    /** 마지막으로 감지(성공/실패 이벤트 포함)된 시간 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float LastStimulusTime = -FLT_MAX;

    /** 마지막으로 '감지 성공'한 시간 (이게 더 중요) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float LastSensedTime = -FLT_MAX;

    /** Forgotten 이벤트가 온 시간(GraceTime 계산용) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float ForgottenTime = -FLT_MAX;

    /** 마지막으로 확인된 자극 위치 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        FVector LastStimulusLocation = FVector::ZeroVector;

    /** 마지막으로 확실히 봤던 위치(시각 성공일 때만 업데이트) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        FVector LastSeenLocation = FVector::ZeroVector;

    /** 마지막 자극이 어떤 센스였나(간단히) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        EThreatSense LastSense = EThreatSense::None;

    /** 라인 오브 사이트 (네가 직접 Trace로 갱신) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        bool bHasLOS = false;

    /** 현재 거리(모니터에서 갱신) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float Distance = FLT_MAX;

    /** 직전 프레임(모니터) 거리 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float PrevDistance = FLT_MAX;

    /** 다가오는 중인가(거리 변화로 계산) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        bool bApproaching = false;

    /** 위협 점수 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float ThreatScore = 0.f;

    /** 타겟 스위칭 흔들림 방지용: 마지막으로 CurrentTarget이 된 시간 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float LastBecamePrimaryTime = -FLT_MAX;

    /** 특정 대상에 대한 “최소 유지 시간” 정책에 사용 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float MinHoldUntilTime = -FLT_MAX;

    /** 이 대상에 대한 쿨다운(예: 방금 놓쳐서 잠깐은 재평가 완화) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
        float CooldownUntilTime = -FLT_MAX;

public:
    void ResetRuntimeEval()
    {
        bHasLOS = false;
        Distance = FLT_MAX;
        PrevDistance = FLT_MAX;
        bApproaching = false;
    }
};

class UAIPerceptionComponent;
class UCombatDecisionComponent;

UCLASS()
class UE5PROJECT_API AEnemyBaseAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AEnemyBaseAIController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
	void SetControllerData(UBehaviorTree* BehaviorTree, UBlackboardData* Blackboard);
	void SetCombatPatternData(UDataTable* CombatDT) { CombatPatternDT = CombatDT; }
	const UDataTable* GetCombatPatternData() { return CombatPatternDT; }
	//virtual void OnUnPossess() override;

public:
    UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
        FName Key_Target = "Target";

    UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
        FName Key_TargetLocation = "TargetLocation";

    UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
        FName Key_BehaviorState = "BehaviorState";

    UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
        FName Key_TargetDistance = "TargetDistance";

    UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
        FName Key_TargetApproaching = "TargetApproaching";

protected:
	UPROPERTY(VisibleAnywhere, Category = AI)
		class UBehaviorTree* CachedBT;

	UPROPERTY(VisibleAnywhere, Category = AI)
		class UBlackboardData* CachedBB;

	UFUNCTION()
		void OnTargetPerceptionUpdated_Delegate(AActor* Actor, FAIStimulus Stimulus);

    UFUNCTION()
        void OnTargetPerceptionForgotten_Delegate(AActor* Actor);

	UPROPERTY(VisibleAnywhere, Category = AI)
		TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent = nullptr;

	//UPROPERTY(VisibleAnywhere, Category = AI)
		//TObjectPtr<class UAISenseConfig_Sight> AISenseConfigSight_Combat = nullptr;

	UPROPERTY(VisibleAnywhere, Category = AI)
		TObjectPtr<class UAISenseConfig_Sight> AISenseConfigSight = nullptr;

	UPROPERTY(VisibleAnywhere, Category = AI)
		TObjectPtr<class UAISenseConfig_Hearing> AISenseConfigHearing = nullptr;

	UPROPERTY(VisibleAnywhere, Category = AI)
		TObjectPtr<class UAISenseConfig_Damage> AISenseConfigDamage = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, Category = AI )
		TObjectPtr<UDataTable> CombatPatternDT = nullptr;

#pragma region Behavior State
protected:
    EAIBehaviorState CurrentState = EAIBehaviorState::Normal;

    UPROPERTY(EditDefaultsOnly, Category = "Threat")
        float AlertThreshold = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat")
        float CombatThreshold = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat")
        float MaxThreatScore = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Timer")
        float MonitorInterval = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Memory")
        float GraceTimeAfterForgotten = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Decay")
        float ThreatDecayPerSec = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Weights")
        float MaxDistanceThreat = 30.0f; // 0~SightRadius에서 최대 30

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Weights")
        float ApproachingBonus = 8.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Weights")
        float HearingBonus = 12.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Weights")
        float DamageBonus = 40.0f; // 피격 즉시 Combat에 가깝게

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Switching")
        float MinPrimaryHoldTime = 1.0f; // 최소 1초는 타겟 고정

    UPROPERTY(EditDefaultsOnly, Category = "Threat|Switching")
        float SwitchScoreMargin = 10.0f; // 새 타겟이 이만큼 더 높아야 교체

        // SightRadius를 Config에서 읽어오면 더 좋지만, 여기선 값으로 둠(프로젝트에 맞게 연결)
    UPROPERTY(EditDefaultsOnly, Category = "Threat|Distances")
        float SightRadiusForThreat = 1000.0f;

	TMap<TWeakObjectPtr<AActor>, FThreatEntry> HostileMap;

    TWeakObjectPtr<AActor> PrimaryTarget;

	float ThreatScore = 0.f;

private:
	FTimerHandle HostileMonitorTimerHandle;

	void StartHostileMonitoring();
	void StopHostileMonitoring();
	void HostileMonitor();

    void UpdateEntryFromStimulus(FThreatEntry& Entry, AActor* Actor, const FAIStimulus& Stimulus);
    void UpdateRuntimeEval(FThreatEntry& Entry, APawn* ControlledPawn, AActor* TargetActor);
    void UpdateThreatScore(FThreatEntry& Entry, float DeltaSeconds);

    bool ShouldRemoveEntry(const FThreatEntry& Entry, float Now) const;

    AActor* ChoosePrimaryTarget(float Now);
    void ApplyPrimaryTarget(AActor* NewPrimary, float Now);

    void UpdateBehaviorStateFromPrimary();
    void PushToBlackboard(AActor* TargetActor);

	bool bHostileMonitoring = false;

public:
    //FORCEINLINE FThreatEntry GetHostileEntry(AActor* Hostile) const { return HostileMap.Find(Hostile); }

#pragma endregion Behavior State

#pragma region Combat State
protected:
    UPROPERTY(VisibleAnywhere, Category = AI)
        TObjectPtr<UCombatDecisionComponent> CombatDecisionComponent = nullptr;

public:
    FORCEINLINE UCombatDecisionComponent* GetCombatDecisionComponent() const { return CombatDecisionComponent; }
#pragma endregion Combat State


#pragma region Lock On
public:
    void EnterLockOn(AActor* Target);
    void ExitLockOn();
#pragma endregion Lock On
	//void OnRepeatTimer();

	//FTimerHandle RepeatTimerHandle;
	//float RepeatInterval;
};
