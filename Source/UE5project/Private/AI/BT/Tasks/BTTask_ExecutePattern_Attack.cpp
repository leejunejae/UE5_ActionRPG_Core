// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTask_ExecutePattern_Attack.h"
#include "Characters/CharacterBase.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "Combat/Components/AttackComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Utils/CoreLog.h"

UBTTask_ExecutePattern_Attack::UBTTask_ExecutePattern_Attack()
{
	NodeName = TEXT("Chase");
}

EBTNodeResult::Type UBTTask_ExecutePattern_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	ACharacterBase* ControllingPawn = Cast<ACharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
	if (ControllingPawn == nullptr)
	{
		UE_LOG(Log_AI, Warning, TEXT("[BTTask_ExecutePattern_Chase] Owner Pawn Not Valid"));
		return EBTNodeResult::Failed;
	}

	AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner());

	if (!EnemyController)
	{
		UE_LOG(Log_AI, Warning, TEXT("[BTTask_ExecutePattern_Chase] EnemyController Invalid"));
		return EBTNodeResult::Failed;
	}

	FName PickedAttackPattern = OwnerComp.GetBlackboardComponent()->GetValueAsName(FName(TEXT("CombatPatternID")));

	ControllingPawn->GetAttackComponent()->ExecuteAttack(PickedAttackPattern);

	return EBTNodeResult::Succeeded;
}
