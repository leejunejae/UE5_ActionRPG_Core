// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PBEnemy_ChaseTarget_Task.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UPBEnemy_ChaseTarget_Task : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UPBEnemy_ChaseTarget_Task();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
