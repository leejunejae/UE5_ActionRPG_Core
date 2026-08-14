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
    // 무기와 무관한 기본 애니메이션. AnimList의 비어 있지 않은 필드만 이를 덮어쓴다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Common")
        FPlayerAnimSet CommonAnimSet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TMap<EWeaponType, FPlayerAnimSet> AnimList;

   const FPlayerAnimSet* FindPlayerAnimSet(const EWeaponType& WeaponType, bool bLogNotFound = false) const;
   FPlayerAnimSet ResolvePlayerAnimSet(const EWeaponType& WeaponType) const;
};
