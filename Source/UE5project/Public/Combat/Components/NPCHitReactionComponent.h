// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Components/HitReactionComponent.h"
#include "NPCHitReactionComponent.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UNPCHitReactionComponent : public UHitReactionComponent
{
	GENERATED_BODY()
	
public:
	UNPCHitReactionComponent();

	EHitResponse EvaluateHitResponse(const FAttackRequest& AttackRequest) override;
};
