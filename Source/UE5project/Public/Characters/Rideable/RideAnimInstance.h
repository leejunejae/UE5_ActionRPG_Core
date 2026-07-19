// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 기본 헤더
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Characters/Rideable/Ride.h"

// 구조체, 자료형
#include "RideAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API URideAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	URideAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	class ARide* Character = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		float Direction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		float TurnRate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		float Vertical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		float Horizontal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		bool IsBreak;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		bool bBraking;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		bool bQuickTurn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character, Meta = (AllowPrivateAccess = true))
		ERideGait CurrentGait = ERideGait::Idle;
};
