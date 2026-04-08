// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTaskNode_ClearFocus.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTaskNode_ClearFocus::UBTTaskNode_ClearFocus()
{
	NodeName = TEXT("ClearFocus");
}

EBTNodeResult::Type UBTTaskNode_ClearFocus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	auto ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (nullptr == ControllingPawn)
		return EBTNodeResult::Failed;

	auto ControllingAI = OwnerComp.GetAIOwner();
	auto Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(FName(TEXT("Target"))));
	if (nullptr == Target)
		return EBTNodeResult::Failed;

	ControllingAI->ClearFocus(EAIFocusPriority::Gameplay);
	return EBTNodeResult::Succeeded;
}