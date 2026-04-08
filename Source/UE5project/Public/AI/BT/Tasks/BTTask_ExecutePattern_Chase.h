// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExecutePattern_Chase.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBTTask_ExecutePattern_Chase : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_ExecutePattern_Chase();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
