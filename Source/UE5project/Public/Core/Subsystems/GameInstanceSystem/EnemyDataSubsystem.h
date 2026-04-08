// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Characters/Enemies/Data/EnemyData.h"
#include "EnemyDataSubsystem.generated.h"

/**
 * 
 */

UCLASS()
class UE5PROJECT_API UEnemyDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere)
		TObjectPtr<UDataTable> EnemyInfoList = nullptr;

	UPROPERTY(EditAnywhere)
		TObjectPtr<UDataTable> EnemyStatsList = nullptr;

	TMap<FName, FEnemyDataSet> EnemyDataCache;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const FEnemyDataSet* GetEnemyData(const FName& EnemyID);
};
