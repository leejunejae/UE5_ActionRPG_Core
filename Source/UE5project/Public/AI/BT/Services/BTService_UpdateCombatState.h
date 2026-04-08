// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCombatState.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBTService_UpdateCombatState : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_UpdateCombatState();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
    UPROPERTY(EditAnywhere, Category = "Blackboard")
        FBlackboardKeySelector CombatStateKey; // Enum

    UPROPERTY(EditAnywhere, Category = "Blackboard")
        FBlackboardKeySelector PatternIDKey;  // Name
};
