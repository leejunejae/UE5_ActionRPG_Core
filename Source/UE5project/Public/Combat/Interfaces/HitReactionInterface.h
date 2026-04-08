// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/Data/CombatData.h"
#include "HitReactionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UHitReactionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UE5PROJECT_API IHitReactionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		//float GetHealth();

	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		//float GetMaxHealth();

	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		//float Heal(float amount);

	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		//void TakeDamage(FAttackInfo DamageInfo);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void OnHit(const FAttackRequest& AttackInfo);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void OnDeathEnd();
};
