// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Data/PlayerAnimSetDataAsset.h"

const FPlayerAnimSet* UPlayerAnimSetDataAsset::FindPlayerAnimSet(const EWeaponType& WeaponType, bool bLogNotFound) const
{
	const FPlayerAnimSet* Found = AnimList.Find(WeaponType);
	if (Found)
	{
		return Found;
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Not SkillInfo"))
	}

	return nullptr;
}