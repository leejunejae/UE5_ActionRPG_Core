// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckBehaviorState.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBTDecorator_CheckBehaviorState : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_CheckBehaviorState();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
