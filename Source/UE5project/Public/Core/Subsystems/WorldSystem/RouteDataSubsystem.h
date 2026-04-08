// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Utils/CoreLog.h"
#include "AI/PatrolRoute.h"
#include "RouteDataSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API URouteDataSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
		TMap<FGuid, TWeakObjectPtr<APatrolRoute>> RouteMap;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	void RegisterRoute(APatrolRoute* Route);
	void DeRegisterRoute(APatrolRoute* Route);
	APatrolRoute* FindRoute(const FGuid& RouteID) const;
};
