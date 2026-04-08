// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/PatrolRoute.h"
#include "Core/Subsystems/WorldSystem/RouteDataSubsystem.h"

// Sets default values
APatrolRoute::APatrolRoute()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	RouteSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolRoute"));
}

// Called when the game starts or when spawned
void APatrolRoute::BeginPlay()
{
	Super::BeginPlay();

	if (auto* RouteSystem = GetWorld()->GetSubsystem<URouteDataSubsystem>())
	{
		RouteSystem->RegisterRoute(this);
	}
}

void APatrolRoute::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* RouteSystem = GetWorld()->GetSubsystem<URouteDataSubsystem>())
	{
		RouteSystem->DeRegisterRoute(this);
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void APatrolRoute::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

int32 APatrolRoute::CalculatePatrolIndex(int32 Current, bool& bIncrease)
{
	int32 PatrolPointNum = RouteSpline ? RouteSpline->GetNumberOfSplinePoints() : 0;

	if (PatrolPointNum <= 0) return INDEX_NONE;

	int32 Direction = bIncrease ? 1 : -1;

	int32 NextIndex = Current + Direction;

	if (NextIndex >= PatrolPointNum)
	{
		bIncrease = false;
		NextIndex = FMath::Clamp(PatrolPointNum - 2, 0, PatrolPointNum - 1);
	}
	else if(NextIndex < 0)
	{
		bIncrease = true;
		NextIndex = (PatrolPointNum >= 2) ? 1 : 0;
	}
	return NextIndex;
}

FVector APatrolRoute::GetPatrolTargetLocationByIndex(int32 SplineIndex)
{
	return RouteSpline->GetLocationAtSplinePoint(SplineIndex, ESplineCoordinateSpace::World);
}

#if WITH_EDITOR

void APatrolRoute::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!RouteID.IsValid())
		RouteID = FGuid::NewGuid();
}
void APatrolRoute::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	if (!bDuplicateForPIE)
	{
		RouteID = FGuid::NewGuid(); // 복제 중복 방지
	}
}

void APatrolRoute::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!RouteID.IsValid())
	{
		RouteID = FGuid::NewGuid();
	}
}
#endif
