// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/PBEnemy_SetMovementSpeed_Task.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "Characters/CharacterBase.h"
#include "Utils/CoreLog.h"


UPBEnemy_SetMovementSpeed_Task::UPBEnemy_SetMovementSpeed_Task()
{
	NodeName = TEXT("SetMovementMode");
}

EBTNodeResult::Type UPBEnemy_SetMovementSpeed_Task::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	auto ControllingPawn = Cast<ACharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
	if (nullptr == ControllingPawn)
		return EBTNodeResult::Failed;

	UE_LOG(Log_Check, Log, TEXT("[BTTask_SetMovementSpeed] Task Executed by %s"), *ControllingPawn->GetName());

	ControllingPawn->SetCurLocomotionGait(LocomotionGait);
	return EBTNodeResult::Succeeded;
}