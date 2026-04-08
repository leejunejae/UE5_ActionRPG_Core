// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "PlayerConfig.generated.h"

/**
 * 
 */
class UActionWindowRules;
class UInputConfigDataAsset;
class UPlayerAttackDataAsset;
class UHitReactionDataAsset;

UCLASS()
class UE5PROJECT_API UPlayerConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere) TSubclassOf<UAnimInstance> AnimBP;
	UPROPERTY(EditAnywhere) TObjectPtr<UActionWindowRules> WindowRules;
	UPROPERTY(EditAnywhere) TObjectPtr<UPlayerAttackDataAsset> AttackData;
	UPROPERTY(EditAnywhere) TObjectPtr<UHitReactionDataAsset> HitReactData;
};
