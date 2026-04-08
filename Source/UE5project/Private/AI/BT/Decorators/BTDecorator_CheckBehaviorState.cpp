// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Decorators/BTDecorator_CheckBehaviorState.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_CheckBehaviorState::UBTDecorator_CheckBehaviorState()
{
	NodeName = TEXT("CheckBehaviorState");
}

bool UBTDecorator_CheckBehaviorState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

	return false;
}