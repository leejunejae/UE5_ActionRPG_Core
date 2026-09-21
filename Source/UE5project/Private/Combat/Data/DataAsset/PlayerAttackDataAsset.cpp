// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/DataAsset/PlayerAttackDataAsset.h"

const FAttackContextSet* UPlayerAttackDataAsset::FindPlayerAttackContext(FGameplayTag CombatStyle, bool bLogNotFound) const
{
	if (const FAttackContextSet* Found = CombatStyleAttackContextMap.Find(CombatStyle)) return Found;
	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("No player attack context for CombatStyle: %s"), *CombatStyle.ToString());
	}
	return nullptr;
}
