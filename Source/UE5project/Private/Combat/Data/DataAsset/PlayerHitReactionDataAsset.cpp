// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/DataAsset/PlayerHitReactionDataAsset.h"

UHitReactionDataAsset* UPlayerHitReactionDataAsset::FindHitReactionDA(FGameplayTag CombatStyle, bool bLogNotFound) const
{
    if (const TObjectPtr<UHitReactionDataAsset>* Found = CombatStyleHitReactionMap.Find(CombatStyle)) return *Found;
    if (bLogNotFound)
    {
        UE_LOG(LogTemp, Error, TEXT("No hit reaction data for CombatStyle: %s"), *CombatStyle.ToString());
    }
    return nullptr;
}
