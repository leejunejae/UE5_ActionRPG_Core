// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Mode/AnimModeBase.h"
#include "AnimMode_Ride.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UAnimMode_Ride : public UAnimModeBase
{
	GENERATED_BODY()
	
public:
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateRideLocomotionIK(float DeltaSeconds);
};
