// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnvQueryContext_AttackTarget.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

UEnvQueryContext_AttackTarget::UEnvQueryContext_AttackTarget()
{

}

void UEnvQueryContext_AttackTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	Super::ProvideContext(QueryInstance, ContextData);

	AActor* QueryOwner = Cast<AActor>(QueryInstance.Owner.Get());
	APawn* Player = UGameplayStatics::GetPlayerPawn(QueryOwner->GetWorld(), 0);
	APlayerController* ControllingPawn= UGameplayStatics::GetPlayerController(QueryOwner->GetWorld(), 0);
	if (ControllingPawn)
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Cast<AActor>(Player));
	}
}