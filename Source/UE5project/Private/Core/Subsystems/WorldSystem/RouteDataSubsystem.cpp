// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/WorldSystem/RouteDataSubsystem.h"
#include "Utils/CoreLog.h"


void URouteDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(Log_Route, Log, TEXT("[RouteDataSubsystem] Initialize (World=%s, Type=%d)"),
		*GetWorld()->GetName(), (int32)GetWorld()->WorldType);
}

void URouteDataSubsystem::Deinitialize()
{
	UE_LOG(Log_Route, Log, TEXT("[RouteDataSubsystem] Deinitialize (World=%s)"), *GetWorld()->GetName());
	Super::Deinitialize();
}

void URouteDataSubsystem::RegisterRoute(APatrolRoute* Route)
{
	if (!Route || !Route->GetRouteID().IsValid()) return;

	UE_LOG(Log_Route, Log, TEXT("[RouteDataSubsystem] Route Registered (ID=%s)"), *Route->GetRouteID().ToString());

	RouteMap.Add(Route->GetRouteID(), Route);
}

void URouteDataSubsystem::DeRegisterRoute(APatrolRoute* Route)
{
	if (!Route) return;

	RouteMap.Remove(Route->GetRouteID());
}

APatrolRoute* URouteDataSubsystem::FindRoute(const FGuid& RouteID) const
{
	if (const TWeakObjectPtr<APatrolRoute>* Found = RouteMap.Find(RouteID))
	{
		UE_LOG(Log_Route, Log, TEXT("[RouteDataSubsystem] Route Found (ID=%s)"), *RouteID.ToString());
		return Found->Get();
	}

	UE_LOG(Log_Route, Warning, TEXT("[RouteDataSubsystem] Route Does Not Found (ID=%s)"), *RouteID.ToString());
	return nullptr;
}