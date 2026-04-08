// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Data/AttackData.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "PlayerAttackDataAsset.generated.h"

/**
 * 
 */

UCLASS()
class UE5PROJECT_API UPlayerAttackDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TMap<EWeaponType, FAttackContextSet> AttackContextMap;

	FAttackContextSet FindPlayerAttackContext(const EWeaponType& WeaponType, bool bLogNotFound = false) const;
};
