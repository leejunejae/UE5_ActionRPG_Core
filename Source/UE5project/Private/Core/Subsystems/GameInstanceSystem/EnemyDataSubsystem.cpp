// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/EnemyDataSubsystem.h"

void UEnemyDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TSoftObjectPtr<UDataTable> EnemyInfoTableRef = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/03_Enemy/Data/EnemyInfo_DT.EnemyInfo_DT")));
	if (EnemyInfoTableRef.IsValid() == false)
	{
		EnemyInfoTableRef.LoadSynchronous();
	}
	EnemyInfoList = EnemyInfoTableRef.Get();

	TSoftObjectPtr<UDataTable> EnemyStatsTableRef = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/03_Enemy/Data/EnemyStats_DT.EnemyStats_DT")));
	if (EnemyStatsTableRef.IsValid() == false)
	{
		EnemyStatsTableRef.LoadSynchronous();
	}
	EnemyStatsList = EnemyStatsTableRef.Get();
}

void UEnemyDataSubsystem::Deinitialize()
{
	EnemyDataCache.Empty();
	Super::Deinitialize();
}

const FEnemyDataSet* UEnemyDataSubsystem::GetEnemyData(const FName& EnemyID)
{
	if (EnemyDataCache.Contains(EnemyID))
	{
		return EnemyDataCache.Find(EnemyID);
	}

	if (!EnemyStatsList || !EnemyInfoList)
	{
		UE_LOG(LogTemp, Error, TEXT("Enemy Data not found for ID: %s"), *EnemyID.ToString());
		return nullptr;
	}

	FEnemyStats* NewEnemyStats = EnemyStatsList->FindRow<FEnemyStats>(EnemyID, TEXT(""));
	FEnemyInfo* NewEnemyInfo = EnemyInfoList->FindRow<FEnemyInfo>(EnemyID, TEXT(""));

	ensure(NewEnemyStats && NewEnemyInfo);

	FEnemyDataSet NewEnemyData(NewEnemyInfo, NewEnemyStats);

	EnemyDataCache.Add(EnemyID, NewEnemyData);

	return &EnemyDataCache[EnemyID];
}