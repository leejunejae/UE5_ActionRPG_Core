// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/DataAsset/PlayerHitReactionDataAsset.h"

UHitReactionDataAsset* UPlayerHitReactionDataAsset::FindHitReactionDA(const EWeaponType& WeaponType, bool bLogNotFound) const
{
    if (const TObjectPtr<UHitReactionDataAsset>* Found = HitReactionMap.Find(WeaponType))
        return *Found;

    if (bLogNotFound)
        UE_LOG(LogTemp, Error, TEXT("Not HitReaction DA"));
    return nullptr;
}