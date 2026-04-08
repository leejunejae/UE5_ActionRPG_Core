// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_Test.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UEnvQueryContext_Test : public UEnvQueryContext
{
	GENERATED_BODY()
	

public:
	UEnvQueryContext_Test();

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
