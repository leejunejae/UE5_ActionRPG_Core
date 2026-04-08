// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PatrolSpline.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBTTask_PatrolSpline : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_PatrolSpline();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
		int32 PatrolRouteIndex = 0;

	bool bDirectionUpward = true;
};
