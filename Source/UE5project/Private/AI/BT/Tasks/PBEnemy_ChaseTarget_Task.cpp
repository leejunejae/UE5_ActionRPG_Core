// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/PBEnemy_ChaseTarget_Task.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UPBEnemy_ChaseTarget_Task::UPBEnemy_ChaseTarget_Task()
{
	NodeName = TEXT("ChaseTarget");
}

EBTNodeResult::Type UPBEnemy_ChaseTarget_Task::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	auto ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UObject* TargetBase = OwnerComp.GetBlackboardComponent()->GetValueAsObject(FName(TEXT("Target")));
	AActor* Target = Cast<AActor>(TargetBase);
	if (Target!=nullptr)
	{
		OwnerComp.GetAIOwner()->MoveToActor(Target, 150.0f);
		//OwnerComp.GetBlackboardComponent()->SetValueAsObject(FName(TEXT("Target")), nullptr);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}