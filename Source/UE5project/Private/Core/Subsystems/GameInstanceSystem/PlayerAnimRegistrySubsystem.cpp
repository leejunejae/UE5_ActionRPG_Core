// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/PlayerAnimRegistrySubsystem.h"

void UPlayerAnimRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TSoftObjectPtr<UPlayerAnimSetDataAsset> AnimSetRef = TSoftObjectPtr<UPlayerAnimSetDataAsset>(FSoftObjectPath(TEXT("/Game/00_Character/Data/AnimData/PlayerAnimSet_DA.PlayerAnimSet_DA")));
	if (AnimSetRef.IsValid() == false)
	{
		AnimSetRef.LoadSynchronous();
	}

	PlayerAnimAsset = AnimSetRef.Get();
}

const FPlayerAnimSet* UPlayerAnimRegistrySubsystem::GetPlayerAnimSet(const EWeaponType& WeaponType) const
{
	if (PlayerAnimAsset)
	{
		return PlayerAnimAsset->FindPlayerAnimSet(WeaponType, true);
	}

	return nullptr;
}
