// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Mode/AnimModeBase.h"
#include "AnimMode_Ladder.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UAnimMode_Ladder : public UAnimModeBase
{
	GENERATED_BODY()
	
public:
	virtual void Tick(float DeltaSeconds) override;
};
