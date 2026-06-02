// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTask_ExecutePattern_Alert.h"
#include "GameFramework/Character.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "EnvironmentQuery/EnvQueryManager.h"

#include "Utils/CoreLog.h"

UBTTask_ExecutePattern_Alert::UBTTask_ExecutePattern_Alert()
{
    NodeName = TEXT("Alert");
    bCreateNodeInstance = true;
    bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_ExecutePattern_Alert::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::ExecuteTask(OwnerComp, NodeMemory);

    OwnerCompRef = &OwnerComp;

    AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner());
    if (!EnemyController)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] EnemyController Invalid"));
        return EBTNodeResult::Failed;
    }

    ACharacter* Character = Cast<ACharacter>(EnemyController->GetPawn());
    if (!Character)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] Owner Pawn Not Valid"));
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    const UDataTable* PatternTable = EnemyController->GetCombatPatternData();
    if (!PatternTable)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] No pattern table"));
        return EBTNodeResult::Failed;
    }

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyController->Key_Target));
    if (!Target)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] No target"));
        return EBTNodeResult::Failed;
    }

    // --- DataTable에서 AlertData 조회 ---
    const FName PatternID = BB->GetValueAsName(FName("CombatPatternID"));
    const FCombatPattern* Row = PatternTable->FindRow<FCombatPattern>(PatternID, TEXT("BTTask_ExecuteAlert::ExecuteTask"));

    if (!Row || Row->ActionType != ECombatActionType::Alert)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] Pattern %s is not Alert or not found"),
            *PatternID.ToString());
        return EBTNodeResult::Failed;
    }

    CachedAlertData = Row->AlertData;

    if (!CachedAlertData.EQSQuery)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] EQS Query not set in pattern: %s"), *PatternID.ToString());
        return EBTNodeResult::Failed;
    }

    // --- 이동 설정 ---
    ApplyMovementSettings(Character, CachedAlertData);

    // --- MoveTo 완료 델리게이트 연결 ---
    EnemyController->ReceiveMoveCompleted.AddDynamic(this, &UBTTask_ExecutePattern_Alert::OnMoveCompleted);

    // --- 타임아웃 타이머 설정 (EQS + MoveTo 전체 시간 커버) ---
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TimeoutTimerHandle,
            this, &UBTTask_ExecutePattern_Alert::OnTimeout,
            CachedAlertData.MaxDuration,
            false);
    }

    // --- EQS 비동기 실행 ---
    FEnvQueryRequest QueryRequest(CachedAlertData.EQSQuery, Cast<UObject>(Character));
    EQSRequestID = QueryRequest.Execute(
        EEnvQueryRunMode::RandomBest25Pct,
        this,
        &UBTTask_ExecutePattern_Alert::OnEQSQueryFinished);

    if (EQSRequestID == INDEX_NONE)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] EQS Request failed"));
        return EBTNodeResult::Failed;
    }

    UE_LOG(Log_AI_Task_Combat_Alert, Log, TEXT("[Task_Alert] %s | EQS started"), *PatternID.ToString());

    return EBTNodeResult::InProgress;
}

// ============================================================
// EQS 결과 콜백
// ============================================================
void UBTTask_ExecutePattern_Alert::OnEQSQueryFinished(TSharedPtr<FEnvQueryResult> QueryResult)
{
    UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] === OnEQSQueryFinished CALLED ==="));  // ← 추가

    EQSRequestID = INDEX_NONE;

    if (!OwnerCompRef)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] OwnerCompRef null"));  // ← 추가
        return;
    }

    if (!QueryResult.IsValid())
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] QueryResult invalid"));  // ← 추가
        FinishLatentTask(*OwnerCompRef, EBTNodeResult::Failed);
        return;
    }

    UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] EQS Items: %d, Success: %d"),  // ← 추가
        QueryResult->Items.Num(),
        QueryResult->IsSuccessful() ? 1 : 0);

    if (!QueryResult.IsValid() || !QueryResult->IsSuccessful() || QueryResult->Items.Num() == 0)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] EQS produced no result"));
        FinishLatentTask(*OwnerCompRef, EBTNodeResult::Failed);
        return;
    }

    AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(OwnerCompRef->GetAIOwner());
    if (!EnemyController)
    {
        FinishLatentTask(*OwnerCompRef, EBTNodeResult::Failed);
        return;
    }

    const FVector DestLocation = QueryResult->GetItemAsLocation(0);

    // --- MoveTo 요청 ---
    FAIMoveRequest MoveReq;
    MoveReq.SetGoalLocation(DestLocation);
    MoveReq.SetAcceptanceRadius(CachedAlertData.AcceptanceRadius);
    MoveReq.SetUsePathfinding(true);
    MoveReq.SetCanStrafe(true);

    FPathFollowingRequestResult MoveResult = EnemyController->MoveTo(MoveReq);
    
    if (ACharacter* Character = Cast<ACharacter>(EnemyController->GetPawn()))
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] MoveTo result code: %d, MoveId: %d, Dest: %s, MyLoc: %s"),
            (int32)MoveResult.Code,
            MoveResult.MoveId.GetID(),
            *DestLocation.ToString(),
            *Character->GetActorLocation().ToString());
    }

    if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] AlreadyAtGoal — finishing immediately"));
        FinishLatentTask(*OwnerCompRef, EBTNodeResult::Succeeded);
        return;
    }
    else if (MoveResult.Code == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] MoveTo Failed"));
        FinishLatentTask(*OwnerCompRef, EBTNodeResult::Failed);
        return;
    }

    CurrentMoveRequestID = MoveResult.MoveId;

    UE_LOG(Log_AI_Task_Combat_Alert, Log, TEXT("[Task_Alert] EQS done -> MoveTo %s"), *DestLocation.ToString());
}

// ============================================================
// MoveTo 완료 콜백
// ============================================================
void UBTTask_ExecutePattern_Alert::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] OnMoveCompleted: incoming ID=%d, our ID=%d"),
        RequestID.GetID(), CurrentMoveRequestID.GetID());

    if (RequestID != CurrentMoveRequestID)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] Ignoring (different ID)"));
        return;
    }

    const bool bSuccess = (Result == EPathFollowingResult::Success) || (Result == EPathFollowingResult::Aborted);

    UE_LOG(Log_AI_Task_Combat_Alert, Log, TEXT("[Task_Alert] MoveCompleted: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));

    if (OwnerCompRef)
    {
        FinishLatentTask(*OwnerCompRef, bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
    }
}

// ============================================================
// 타임아웃
// ============================================================
void UBTTask_ExecutePattern_Alert::OnTimeout()
{
    if (!OwnerCompRef) return;

    UE_LOG(Log_AI_Task_Combat_Alert, Log, TEXT("[Task_Alert] Timeout — finishing task"));

    if (AEnemyBaseAIController* AIC = Cast<AEnemyBaseAIController>(OwnerCompRef->GetAIOwner()))
    {
        AIC->StopMovement();
    }

    FinishLatentTask(*OwnerCompRef, EBTNodeResult::Succeeded);
}

// ============================================================
// OnTaskFinished — 모든 정리 (Reposition과 동일 패턴)
// ============================================================
void UBTTask_ExecutePattern_Alert::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    UE_LOG(Log_AI_Task_Combat_Alert, Verbose, TEXT("[Task_Alert] === OnTaskFinished: Result=%d ==="), (int32)TaskResult);

    // EQS 진행 중이면 취소
    if (EQSRequestID != INDEX_NONE)
    {
        if (UEnvQueryManager* QM = UEnvQueryManager::GetCurrent(this))
        {
            QM->AbortQuery(EQSRequestID);
        }
        EQSRequestID = INDEX_NONE;
    }

    // 델리게이트 해제
    if (AEnemyBaseAIController* AIC = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner()))
    {
        AIC->ReceiveMoveCompleted.RemoveDynamic(this, &UBTTask_ExecutePattern_Alert::OnMoveCompleted);
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
// 이동 설정 적용
// ============================================================
void UBTTask_ExecutePattern_Alert::ApplyMovementSettings(ACharacter* Character, const FAlertData& Data)
{
    if (!Character) return;

    UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
    if (!MovementComp) return;

    MovementComp->MaxWalkSpeed = Data.MovementSpeed;
}