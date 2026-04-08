// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Components/HitReactionComponent.h"
#include "Characters/Interfaces/StatInterface.h"

#include "PlayerHitReactionComponent.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UPlayerHitReactionComponent : public UHitReactionComponent
{
	GENERATED_BODY()

public:
	UPlayerHitReactionComponent();

	EHitResponse EvaluateHitResponse(const FAttackRequest& AttackRequest) override;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
