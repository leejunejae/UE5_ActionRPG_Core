// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Services/BTService_UpdateCombatState.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyBase.h"

#include "AI/CombatDecisionComponent.h"
#include "Characters/Enemies/Components/CharacterStatComponent.h"
#include "Utils/CustomMathUtility.h"
#include "Utils/CoreLog.h"

using namespace CustomMath::Spatial;

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
	NodeName = TEXT("UpdateCombatState");
	Interval = 0.5f;                    // 진행 중 패턴 적절성 확인 주기
	RandomDeviation = 0.1f;             // 여러 적의 Tick 싱크 방지

	bNotifyTick = true;
	bCallTickOnSearchStart = true;      // Task 종료 직후 즉시 재평가
}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
    AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner());
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    UWorld* World = GetWorld();

    if (!EnemyController)
    {
        UE_LOG(Log_AI, Warning, TEXT("[BTService_UpdateCombatState] EnemyController Invalid"));
        return;
    }
    if (!Blackboard)
    {
        UE_LOG(Log_AI, Warning, TEXT("[BTService_UpdateCombatState] %s Blackboard Not Set"), *EnemyController->GetName());
        return;
    }
    if (!World)
    {
        UE_LOG(Log_AI, Warning, TEXT("[BTService_UpdateCombatState] World Not Valid"));
        return;
    }

    AEnemyBase* Enemy = Cast<AEnemyBase>(EnemyController->GetPawn());
    const UDataTable* CombatPatternDT = EnemyController->GetCombatPatternData();

    if (!Enemy)
    {
        UE_LOG(Log_AI, Warning, TEXT("[BTService_UpdateCombatState] %s Owner Character Not Valid"), *EnemyController->GetName());
        return;
    }
    if (!CombatPatternDT)
    {
        UE_LOG(Log_AI, Warning, TEXT("[BTService_UpdateCombatState] CombatPattern Invalid"));
        return;
    }


	FCombatContext Context;

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(EnemyController->Key_Target));

	Context.DistanceToTarget = Blackboard->GetValueAsFloat(EnemyController->Key_TargetDistance);
	Context.AbsDeltaYawDeg = AbsDeltaYawDeg(Enemy, Target);
	Context.Aggressiveness = Enemy->Aggressiveness; // EnemyStat에서 가져오기
	Context.HPPercent = Enemy->GetStatComponent()->GetCommonStats().GetHealthPercent();
	Context.CurrentTime = World->GetTimeSeconds();
	Context.bHasLOS = EnemyController->LineOfSightTo(Target);

	Context.bPoiseBroken = false;
	Context.bRangedThreat = false;
	Context.bStanceBroken = false;
	Context.bTargetAttacking = false;
	Context.bTargetGuarding = false;
	Context.bTargetInRecovery = false;
	Context.bTargetPoiseBroken = false;
	Context.bTargetStanceBroken = false;
	Context.Phase = 1;

	const FCombatDecisionResult ExecuteCombatPattern = EnemyController->GetCombatDecisionComponent()->Decide(Context, CombatPatternDT, true, Enemy->EnemyID.ToString());

	Blackboard->SetValueAsEnum(CombatStateKey.SelectedKeyName, (uint8)ExecuteCombatPattern.PickedType);
	Blackboard->SetValueAsName(PatternIDKey.SelectedKeyName, ExecuteCombatPattern.PickedPatternID);
}
