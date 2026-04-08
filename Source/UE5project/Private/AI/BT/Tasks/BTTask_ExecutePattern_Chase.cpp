// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTask_ExecutePattern_Chase.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Utils/CoreLog.h"

UBTTask_ExecutePattern_Chase::UBTTask_ExecutePattern_Chase()
{
	NodeName = TEXT("Chase");
}

EBTNodeResult::Type UBTTask_ExecutePattern_Chase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	auto ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		UE_LOG(Log_AI, Warning, TEXT("[BTTask_ExecutePattern_Chase] Owner Pawn Not Valid"));
		return EBTNodeResult::Failed;
	}

	UObject* TargetBase = OwnerComp.GetBlackboardComponent()->GetValueAsObject(FName(TEXT("Target")));
	AActor* Target = Cast<AActor>(TargetBase);
	if (Target != nullptr)
	{
		OwnerComp.GetAIOwner()->MoveToActor(Target, 150.0f);
		
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}