// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Components/StatComponent.h"
#include "CharacterStatComponent.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UCharacterStatComponent : public UStatComponent
{
	GENERATED_BODY()
	

public:
	FCharacterStats& GetCommonStats() override { return NPCStats.BaseStats; }
	FNPCStats GetNPCStats() const { return NPCStats; }

	void InitializeNPCStats(const FNPCStats& NewStat) { NPCStats = NewStat; }
	
	bool ChangeStance(const float Amount, const EStatChangeType StatChangeType);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats") // 데이터 테이블로 정해진 NPC의 스탯 값
		FNPCStats NPCStats;
};
