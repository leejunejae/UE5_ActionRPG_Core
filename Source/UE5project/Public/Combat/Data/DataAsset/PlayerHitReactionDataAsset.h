// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Data/DataAsset/HitReactionDataAsset.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "PlayerHitReactionDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UPlayerHitReactionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TMap<EWeaponType, TObjectPtr<UHitReactionDataAsset>> HitReactionMap;

    UHitReactionDataAsset* FindHitReactionDA(const EWeaponType& WeaponType, bool bLogNotFound = false) const;
};
