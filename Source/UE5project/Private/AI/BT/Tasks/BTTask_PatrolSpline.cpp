// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTask_PatrolSpline.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "AI/PatrolRoute.h"
#include "Utils/CoreLog.h"

UBTTask_PatrolSpline::UBTTask_PatrolSpline()
{
	NodeName = TEXT("PatrolSpline");
}

EBTNodeResult::Type UBTTask_PatrolSpline::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	auto ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		UE_LOG(Log_AI, Warning, TEXT("[BTTask_PatrolSpline] OwnerPawn Not Valid"));
		return EBTNodeResult::Failed;
	}

	APatrolRoute* TargetRoute = Cast<APatrolRoute>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(FName(TEXT("PatrolRoute"))));

	if (TargetRoute == nullptr)
	{
		UE_LOG(Log_Route, Warning, TEXT("[BTTask_PatrolSpline] [%s] TargetRoute is nullptr"), *GetFullName());
		return EBTNodeResult::Failed;
	}

	PatrolRouteIndex = TargetRoute->CalculatePatrolIndex(PatrolRouteIndex, bDirectionUpward);

	FVector PatrolTargetLocation = TargetRoute->GetPatrolTargetLocationByIndex(PatrolRouteIndex);

	OwnerComp.GetBlackboardComponent()->SetValueAsVector(FName(TEXT("PatrolLocation")), PatrolTargetLocation);

	return EBTNodeResult::Succeeded;
}