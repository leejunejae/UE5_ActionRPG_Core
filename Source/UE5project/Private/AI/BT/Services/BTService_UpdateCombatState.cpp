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
	bNotifyTick = true;

}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const UDataTable* CombatPatternDT = EnemyController->GetCombatPatternData();
	AEnemyBase* Enemy = Cast<AEnemyBase>(EnemyController->GetPawn());
	UWorld* World = GetWorld();

	if (!EnemyController)
	{
		UE_LOG(Log_AI, Warning, TEXT("[EnemyBaseAIController] EnemyController Invalid"));
		return;
	}

	if (!Enemy)
	{
		UE_LOG(Log_AI, Warning, TEXT("[EnemyBaseAIController] %s Owner Character Not Valid"), *EnemyController->GetName());
		return;
	}

	if (!Blackboard)
	{
		UE_LOG(Log_AI, Warning, TEXT("[EnemyBaseAIController] %s Blackboard Not Set"), *EnemyController->GetName());
		return;
	}

	if(!World)
	{
		UE_LOG(Log_AI, Warning, TEXT("[EnemyBaseAIController] World Not Valid"));
		return;
	}

	if (!CombatPatternDT)
	{
		UE_LOG(Log_AI, Warning, TEXT("[EnemyBaseAIController] CombatPattern Invalid"));
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

	const FCombatDecisionResult ExecuteCombatPattern = EnemyController->GetCombatDecisionComponent()->Decide(Context, CombatPatternDT, true);

	Blackboard->SetValueAsEnum(CombatStateKey.SelectedKeyName, (uint8)ExecuteCombatPattern.PickedType);
	Blackboard->SetValueAsName(PatternIDKey.SelectedKeyName, ExecuteCombatPattern.PickedPatternID);
}
