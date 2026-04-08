// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BaseCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UBaseCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadOnly, Category = "Ground")
        float GroundDistance = BIG_NUMBER;

    UPROPERTY(EditAnywhere, Category = "Ground")
        float GroundTraceDistance = 300.f;

    UFUNCTION(BlueprintCallable, Category = "Ground")
        float GetGroundDistance(bool bForceUpdate = false);

private:
    uint64 LastUpdateFrame = 0;
    double LastUpdateTime = -1.0;
    double SampleIntervalSec = 0.0; // 0이면 매 프레임 허용, 0.02 등으로 제한 가능
    void UpdateGroundDistance();
};
