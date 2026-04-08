// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "EnemyAIDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UEnemyAIDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    // 사용하는 블랙보드
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
        TObjectPtr<UBlackboardData> EnemyBlackboard;

    // 사용하는 비헤이비어 트리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
        TObjectPtr<UBehaviorTree> EnemyBehaviorTree;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
        TObjectPtr<UDataTable> CombatDesci;
};
