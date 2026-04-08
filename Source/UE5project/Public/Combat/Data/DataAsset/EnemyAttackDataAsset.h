// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Data/AttackData.h"
#include "EnemyAttackDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UEnemyAttackDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FAttackContextSet AttackContextSet;
};
