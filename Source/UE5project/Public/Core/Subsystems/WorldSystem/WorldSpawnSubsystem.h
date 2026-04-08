// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSpawnSubsystem.generated.h"

/**
 * 
 */
class ASpawnMarker;
class UEnemyDataSubsystem;

UCLASS()
class UE5PROJECT_API UWorldSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/* === 마커 등록/해제 === */
	void RegisterMarker(ASpawnMarker* Marker);
	void UnregisterMarker(ASpawnMarker* Marker);

	void SpawnFromMarker(ASpawnMarker* Marker);

protected:
	UPROPERTY(Transient)
		TMap<FGuid, TWeakObjectPtr<ASpawnMarker>> MarkersBySpawnID;
};
