// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnvQueryContext_AttackTarget.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Characters/Enemies/EnemyBaseAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

#include "Utils/CoreLog.h"

UEnvQueryContext_AttackTarget::UEnvQueryContext_AttackTarget()
{

}

void UEnvQueryContext_AttackTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	Super::ProvideContext(QueryInstance, ContextData);

    APawn* QuerierPawn = Cast<APawn>(QueryInstance.Owner.Get());
    if (!QuerierPawn)
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] QuerierPawn null"));
        return;
    }
    UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Querier: %s"), *QuerierPawn->GetName());

    AActor* Target = nullptr;

    if (AEnemyBaseAIController* Controller = Cast<AEnemyBaseAIController>(QuerierPawn->GetController()))
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Controller OK"));

        if (UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent())
        {
            UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] BB OK, Key_Target: %s"),
                *Controller->Key_Target.ToString());

            Target = Cast<AActor>(Blackboard->GetValueAsObject(Controller->Key_Target));

            if (Target)
            {
                UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Target from BB: %s"), *Target->GetName());
            }
            else
            {
                UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Target from BB: NULL"));
            }
        }
        else
        {
            UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Blackboard NULL"));
        }
    }
    else
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Controller cast failed"));
    }

    if (!Target)
    {
            UGameplayStatics::GetPlayerPawn(QuerierPawn->GetWorld(), 0);
        UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Fallback to player: %s"),
            Target ? *Target->GetName() : TEXT("NULL"));
    }

    if (Target)
    {
        UEnvQueryItemType_Actor::SetContextHelper(ContextData, Target);
        UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] Final Target SET: %s"), *Target->GetName());
    }
    else
    {
        UE_LOG(Log_AI_Task_Combat_Alert, Warning, TEXT("[Context_AttackTarget] === NO TARGET — Context FAILED ==="));
    }
}