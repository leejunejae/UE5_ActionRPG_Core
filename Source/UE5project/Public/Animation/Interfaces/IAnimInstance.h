// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Characters/Data/IKData.h"
#include "IAnimInstance.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIAnimInstance : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UE5PROJECT_API IIAnimInstance
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void ResetTurn();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void ResetHitAir();

	UFUNCTION(BlueprintNativeEvent, Category = "IK")
		void SetIKPhaseAlpha(FGameplayTag TargetIKPhase, float Weight);

	UFUNCTION(BlueprintNativeEvent, Category = "IK")
		float GetIKPhaseAlpha(FGameplayTag TargetIKPhase);

	UFUNCTION(BlueprintNativeEvent, Category = "IK")
		void SetIKLayerAlpha(FGameplayTag TargetIKLayer, ELimbList Limb, float Weight);

	UFUNCTION(BlueprintNativeEvent, Category = "IK")
		float GetIKLayerAlpha(FGameplayTag TargetIKLayer, ELimbList Limb);
};
