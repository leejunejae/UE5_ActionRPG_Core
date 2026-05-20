// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/Tasks/BTTask_ExecutePattern_Attack.h"
#include "Characters/CharacterBase.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "Combat/Components/AttackComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Utils/CoreLog.h"

UBTTask_ExecutePattern_Attack::UBTTask_ExecutePattern_Attack()
{
	NodeName = TEXT("Attack");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ExecutePattern_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	AEnemyBaseAIController* EnemyController = Cast<AEnemyBaseAIController>(OwnerComp.GetAIOwner());

	if (!EnemyController)
	{
		UE_LOG(Log_AI, Warning, TEXT("[BTTask_ExecutePattern_Reposition] EnemyController Invalid"));
		return EBTNodeResult::Failed;
	}

	ACharacterBase* ControllingPawn = Cast<ACharacterBase>(EnemyController->GetPawn());
	if (ControllingPawn == nullptr)
	{
		UE_LOG(Log_AI, Warning, TEXT("[BTTask_ExecutePattern_Reposition] Owner Pawn Not Valid"));
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyController->Key_Target));
	if (!Target)
	{
		UE_LOG(Log_AI, Warning, TEXT("[Reposition] No target"));
		return EBTNodeResult::Failed;
	}

	UAttackComponent* AttackComp = ControllingPawn->GetAttackComponent();
	if (!AttackComp)
	{
		return EBTNodeResult::Failed;
	}

	OwnerCompRef = &OwnerComp;
	AttackComp->OnAttackFinished.AddUObject(this, &UBTTask_ExecutePattern_Attack::OnAttackFinished);

	FName PickedAttackPattern = OwnerComp.GetBlackboardComponent()->GetValueAsName(FName(TEXT("CombatPatternID")));
	AttackComp->ExecuteAttack(PickedAttackPattern);

	return EBTNodeResult::InProgress;
}

void UBTTask_ExecutePattern_Attack::OnAttackFinished(bool bInterrupted)
{
	UE_LOG(Log_Attack, Log, TEXT("[UAttackComponent] Attack End Delegate"));
	if (OwnerCompRef)
	{
		FinishLatentTask(*OwnerCompRef, EBTNodeResult::Succeeded);
	}
}

void UBTTask_ExecutePattern_Attack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,EBTNodeResult::Type TaskResult)
{
	if (!IsValid(OwnerCompRef)) return;

	ACharacterBase* Pawn = Cast<ACharacterBase>(OwnerCompRef->GetAIOwner()->GetPawn());

	if (!IsValid(Pawn)) return;
	Pawn->GetAttackComponent()->OnAttackFinished.RemoveAll(this);

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}