// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Decorators/BTDecorator_CheckHitReaction.h"

UBTDecorator_CheckHitReaction::UBTDecorator_CheckHitReaction()
{
	NodeName = TEXT("CheckHitReaction");
}

bool UBTDecorator_CheckHitReaction::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	Super::CalculateRawConditionValue(OwnerComp, NodeMemory);



	return false;
}
