// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnvQueryContext_Test.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"


UEnvQueryContext_Test::UEnvQueryContext_Test()
{

}

void UEnvQueryContext_Test::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	Super::ProvideContext(QueryInstance, ContextData);
	AActor* TestPawn = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass());
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, TestPawn);
}