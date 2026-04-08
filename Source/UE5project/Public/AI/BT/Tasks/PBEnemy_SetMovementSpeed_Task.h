// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Core/MovementTypes.h"
#include "PBEnemy_SetMovementSpeed_Task.generated.h"

/**
 * 
 */

UCLASS()
class UE5PROJECT_API UPBEnemy_SetMovementSpeed_Task : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UPBEnemy_SetMovementSpeed_Task();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ExposeOnSpawn = "true", OverrideNativeName = "bOnHand"))
		ELocomotionGait LocomotionGait;
};
