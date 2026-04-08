// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Player/PlayerBaseAnimInstance.h"
#include "FallenKnightAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UFallenKnightAnimInstance : public UPlayerBaseAnimInstance
{
	GENERATED_BODY()

public:
	UFallenKnightAnimInstance();
	//virtual void NativeInitializeAnimation() override;
	//virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/*
	void AnimNotify_NOT_CheckLadderStance();
	void AnimNotify_NOT_EnableInputLock();
	void AnimNotify_NOT_ClimbEnd();
	void AnimNotify_NOT_ClimbStart();
	void AnimNotify_NOT_ResetLadder();
	*/
};
