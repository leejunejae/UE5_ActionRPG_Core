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
class UPlayerHitReactionDataAsset;

USTRUCT(BlueprintType)
struct FCriticalExecutionSettings
{
	GENERATED_BODY()

	/** 일반 공격 입력으로 처형 대상을 탐색할 최대 거리. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution", meta = (ClampMin = "0.0"))
	float SearchRange = 220.0f;

	/** 플레이어 정면을 기준으로 처형 대상을 허용할 좌우 최대 각도. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SearchMaxAngle = 75.0f;
};

UCLASS()
class UE5PROJECT_API UPlayerConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere) TSubclassOf<UAnimInstance> AnimBP;
	UPROPERTY(EditAnywhere) TObjectPtr<UActionWindowRules> WindowRules;
	UPROPERTY(EditAnywhere) TObjectPtr<UPlayerAttackDataAsset> AttackData;
	UPROPERTY(EditAnywhere) TObjectPtr<UPlayerHitReactionDataAsset> HitReactData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Critical Execution")
	FCriticalExecutionSettings CriticalExecution;
};
