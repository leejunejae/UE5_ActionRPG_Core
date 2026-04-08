// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_AttackTarget.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UEnvQueryContext_AttackTarget : public UEnvQueryContext
{
	GENERATED_BODY()
	
public:
	UEnvQueryContext_AttackTarget();

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
