// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTask_ExecutePattern_Reposition.h"
#include "Characters/Enemies/EnemyBaseAIController.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "NavigationSystem.h"

#include "Utils/CoreLog.h"

UBTTask_ExecutePattern_Reposition::UBTTask_ExecutePattern_Reposition()
{
	NodeName = TEXT("Reposition");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ExecutePattern_Reposition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	OwnerCompRef = &OwnerComp;

	AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner());

	if (!EnemyController)
	{
		UE_LOG(Log_AI, Warning, TEXT("[Task__Reposition] EnemyController Invalid"));
		return EBTNodeResult::Failed;
	}

	ACharacter* Character = Cast<ACharacter>(EnemyController->GetPawn());
	if (Character == nullptr)
	{
		UE_LOG(Log_AI, Warning, TEXT("[Task__Reposition] Owner Pawn Not Valid"));
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	const UDataTable* PatternTable = EnemyController->GetCombatPatternData();
	if (!PatternTable)
	{
		UE_LOG(Log_AI, Warning, TEXT("[Task_Reposition] No pattern table"));
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyController->Key_Target));
	if (!Target)
	{
		UE_LOG(Log_AI, Warning, TEXT("[Task_Reposition] No target"));
		return EBTNodeResult::Failed;
	}

	// --- 2. DataTable에서 RepositionData 조회 ---

	const FName PatternID = BB->GetValueAsName(FName("CombatPatternID"));
	const FCombatPattern* Row = PatternTable->FindRow<FCombatPattern>(PatternID, TEXT("BTTask_ExecuteReposition::ExecuteTask"));

	if (!Row || Row->ActionType != ECombatActionType::Reposition)
	{
		UE_LOG(Log_AI, Warning, TEXT("[Task_Reposition] Pattern %s is not Reposition or not found"),
			*PatternID.ToString());
		return EBTNodeResult::Failed;
	}

	// --- 3. 이동 속도---
	const FRepositionData& Data = Row->RepositionData;
	ApplyMovementSettings(Character, Data);

	// --- 4. MoveTo 완료 델리게이트 연결 ---
	EnemyController->ReceiveMoveCompleted.AddDynamic(this, &UBTTask_ExecutePattern_Reposition::OnMoveCompleted);

	// --- 5. 분기: TowardTarget(Engage)이면 Actor 추적, 아니면 Location 이동 ---
	FAIMoveRequest MoveReq;
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetCanStrafe(true);

	if (Data.DirectionMode == ERepositionDirection::TowardTarget)
	{
		// Engage: Actor 추적
		MoveReq.SetGoalActor(Target);
		MoveReq.SetAcceptanceRadius(Data.TargetDistance);

		UE_LOG(Log_AI, Log, TEXT("[Task_Reposition] %s | Engage -> %s (Radius: %.0f)"),
			*PatternID.ToString(), *Target->GetName(), Data.AcceptanceRadius);
	}
	else
	{
		// 고정 위치 이동
		FVector NewLocation;
		const bool bCalcOK = CalculateFixedLocation(
			GetWorld(),
			Data,
			Character->GetActorLocation(),
			Target->GetActorLocation(),
			NewLocation);

		if (!bCalcOK)
		{
			UE_LOG(Log_AI, Warning, TEXT("[Task_Reposition] Location calc failed for %s"),
				*PatternID.ToString());

			// 정리하고 실패 반환
			EnemyController->ReceiveMoveCompleted.RemoveDynamic(this, &UBTTask_ExecutePattern_Reposition::OnMoveCompleted);
			return EBTNodeResult::Failed;
		}

		MoveReq.SetGoalLocation(NewLocation);
		MoveReq.SetAcceptanceRadius(Data.AcceptanceRadius);
		UE_LOG(Log_AI, Log, TEXT("[Task_Reposition] %s | Fixed -> %s (Radius: %.0f)"),
			*PatternID.ToString(), *NewLocation.ToString(), Data.AcceptanceRadius);
	}

	// --- 6. MoveTo 요청 실행 ---
	FPathFollowingRequestResult MoveResult = EnemyController->MoveTo(MoveReq);

	if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		// 이미 도착 — 정리 후 즉시 성공
		EnemyController->ReceiveMoveCompleted.RemoveDynamic(this, &UBTTask_ExecutePattern_Reposition::OnMoveCompleted);
		return EBTNodeResult::Succeeded;
	}
	else if (MoveResult.Code == EPathFollowingRequestResult::Failed)
	{
		// MoveTo 시작 실패 — 정리 후 실패 반환
		EnemyController->ReceiveMoveCompleted.RemoveDynamic(this, &UBTTask_ExecutePattern_Reposition::OnMoveCompleted);
		return EBTNodeResult::Failed;
	}

	// 정상적으로 시작됨 — 콜백 대기
	CurrentMoveRequestID = MoveResult.MoveId;

	// --- 7. 타임아웃 타이머 설정 ---
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TimeoutTimerHandle,
			this, &UBTTask_ExecutePattern_Reposition::OnTimeout,
			Data.MaxDuration,
			false);
	}

	return EBTNodeResult::InProgress;
}

// ============================================================
// MoveTo 완료 콜백
// ============================================================
void UBTTask_ExecutePattern_Reposition::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	// 우리 요청이 아니면 무시
	if (RequestID != CurrentMoveRequestID) return;

	// 타임아웃 타이머 취소
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
	}

	const bool bSuccess = (Result == EPathFollowingResult::Success) || (Result == EPathFollowingResult::Aborted);  // 인터럽트도 정상 종료로 처리

	UE_LOG(Log_AI, Log, TEXT("[Reposition] MoveCompleted: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));

	FinishLatentTask(*OwnerCompRef, bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

// ============================================================
// 타임아웃
// ============================================================
void UBTTask_ExecutePattern_Reposition::OnTimeout()
{
	if (!OwnerCompRef) return;

	UE_LOG(Log_AI, Log, TEXT("[Reposition] Timeout — finishing task"));

	// MoveTo 강제 중단
	if (AEnemyBaseAIController* AIC = Cast<AEnemyBaseAIController>(OwnerCompRef->GetAIOwner()))
	{
		AIC->StopMovement();
	}

	// 타임아웃은 "도달 못 했어도 자연스럽게 종료" — 성공으로 처리
	FinishLatentTask(*OwnerCompRef, EBTNodeResult::Succeeded);
}

// ============================================================
// OnTaskFinished — 정리 처리
// ============================================================
void UBTTask_ExecutePattern_Reposition::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// 델리게이트 해제
	if (AEnemyBaseAIController* AIC = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner()))
	{
		AIC->ReceiveMoveCompleted.RemoveDynamic(this, &UBTTask_ExecutePattern_Reposition::OnMoveCompleted);
	}

	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
	}

	// 참조 정리
	OwnerCompRef = nullptr;
	CurrentMoveRequestID = FAIRequestID();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

// ============================================================
// 정적 헬퍼: 방향 계산
// ============================================================
FVector UBTTask_ExecutePattern_Reposition::ResolveDirection(ERepositionDirection Mode, const FVector& SelfLoc, const FVector& TargetLoc)
{
	// 대상에서 자신을 향한 단위 벡터 (XY 평면)
	FVector TargetToSelf = SelfLoc - TargetLoc;
	TargetToSelf.Z = 0.f;
	TargetToSelf = TargetToSelf.GetSafeNormal();

	// 대상과 자신이 거의 같은 위치 — 폴백
	if (TargetToSelf.IsNearlyZero())
	{
		TargetToSelf = FVector::ForwardVector;
	}

	switch (Mode)
	{
	case ERepositionDirection::AwayFromTarget:
		return TargetToSelf;

	case ERepositionDirection::SideLeft:
		return TargetToSelf.RotateAngleAxis(-90.f, FVector::UpVector);

	case ERepositionDirection::SideRight:
		return TargetToSelf.RotateAngleAxis(90.f, FVector::UpVector);
	case ERepositionDirection::RandomSide:
	{
		const float Angle = FMath::RandBool() ? -90.f : 90.f;
		return TargetToSelf.RotateAngleAxis(Angle, FVector::UpVector);
	}
	case ERepositionDirection::TowardTarget:
		// 사용되지 않음 (호출 전 분기 처리됨) — 안전망
	default:
		return TargetToSelf;
	}
}

// ============================================================
// 정적 헬퍼: 고정 위치 계산
// ============================================================
bool UBTTask_ExecutePattern_Reposition::CalculateFixedLocation( UWorld* World, const FRepositionData& Data, const FVector& SelfLoc, const FVector& TargetLoc, FVector& OutLocation)
{
	// TowardTarget은 여기서 처리하지 않음 (Actor 추적으로 별도 처리)
	if (Data.DirectionMode == ERepositionDirection::TowardTarget)
	{
		return false;
	}

	const FVector Direction = ResolveDirection(Data.DirectionMode, SelfLoc, TargetLoc);
	if (Direction.IsNearlyZero()) return false;

	// 대상 위치 + 방향 * 거리
	const FVector RawLocation = TargetLoc + Direction * Data.TargetDistance;

	// NavMesh 투영
	return ProjectToNavMesh(World, RawLocation, OutLocation);
}

// ============================================================
// 정적 헬퍼: NavMesh 투영
// ============================================================
bool UBTTask_ExecutePattern_Reposition::ProjectToNavMesh(
	UWorld* World,
	const FVector& InLocation,
	FVector& OutLocation)
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys) return false;

	FNavLocation NavLoc;
	const FVector QueryExtent(500.f, 500.f, 500.f);

	if (NavSys->ProjectPointToNavigation(InLocation, NavLoc, QueryExtent))
	{
		OutLocation = NavLoc.Location;
		return true;
	}

	return false;
}

void UBTTask_ExecutePattern_Reposition::ApplyMovementSettings(ACharacter* Character, const FRepositionData& Data)
{
	if (!Character) return;

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp) return;

	MovementComp->MaxWalkSpeed = Data.MovementSpeed;
}