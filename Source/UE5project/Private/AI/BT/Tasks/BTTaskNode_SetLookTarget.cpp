// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTaskNode_SetLookTarget.h"

UBTTaskNode_SetLookTarget::UBTTaskNode_SetLookTarget()
{
	NodeName = TEXT("SetLookTarget");
}

EBTNodeResult::Type UBTTaskNode_SetLookTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	return EBTNodeResult::Succeeded;
}