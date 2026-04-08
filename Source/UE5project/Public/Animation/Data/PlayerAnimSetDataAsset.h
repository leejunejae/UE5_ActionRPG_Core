// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "Animation/Data/AnimData.h"
#include "PlayerAnimSetDataAsset.generated.h"

/**
 * 
 */

UCLASS()
class UE5PROJECT_API UPlayerAnimSetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TMap<EWeaponType, FPlayerAnimSet> AnimList;

   const FPlayerAnimSet* FindPlayerAnimSet(const EWeaponType& WeaponType, bool bLogNotFound = false) const;
};
