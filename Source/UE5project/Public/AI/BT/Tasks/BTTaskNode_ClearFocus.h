// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTaskNode_ClearFocus.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBTTaskNode_ClearFocus : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTaskNode_ClearFocus();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
