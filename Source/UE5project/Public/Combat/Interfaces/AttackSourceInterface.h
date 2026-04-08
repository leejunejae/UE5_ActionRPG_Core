// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/Data/AttackData.h"
#include "AttackSourceInterface.generated.h"

// This class does not need to be modified.

USTRUCT(BlueprintType)
struct FAttackTraceSource
{
    GENERATED_BODY()

public:
    UPROPERTY() USceneComponent* TraceComponent = nullptr; // 무기 메시 or 캐릭터 메시
    UPROPERTY() float Radius = 0.f;

    // 필요하면: 추가 소켓들, 오프셋, 채널, 트레이스 프로파일 등
};

USTRUCT(BlueprintType)
struct FAttackDamageSource
{
    GENERATED_BODY()

public:
    UPROPERTY() float AttackRating = 0.f;   // (무기공격력 * 스탯보정)
    UPROPERTY() float PoiseRating = 0.f;
    UPROPERTY() float StanceRating = 0.f;
};

UINTERFACE(MinimalAPI)
class UAttackSourceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UE5PROJECT_API IAttackSourceInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
        FAttackTraceSource GetAttackTraceSource(EAttackSourceType AttackSourceType) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
        FAttackDamageSource GetAttackDamageSource() const;
};
