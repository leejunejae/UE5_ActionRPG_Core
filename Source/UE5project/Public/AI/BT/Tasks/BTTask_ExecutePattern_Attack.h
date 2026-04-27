// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExecutePattern_Attack.generated.h"

/**
 * 
 */

UCLASS()
class UE5PROJECT_API UBTTask_ExecutePattern_Attack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_ExecutePattern_Attack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	void OnAttackFinished();

	UBehaviorTreeComponent* OwnerCompRef = nullptr;
};
