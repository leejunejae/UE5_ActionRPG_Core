// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Data/DataAsset/EnemyAttackDataAsset.h"
#include "EnemyComponentDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UEnemyComponentDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/** 전투 패턴 리스트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TObjectPtr<UDataTable> CombatPatternData;

	/* 공격 패턴 데이터*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FAttackContextSet AttackContextSet;
};
