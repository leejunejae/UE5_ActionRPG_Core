// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "UObject/ConstructorHelpers.h"

void UWeaponDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

	TSoftObjectPtr<UDataTable> SetsTableRef = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/02_Item/Equipment/Weapon/Data/PlayerWeaponData_DT.PlayerWeaponData_DT")));
	if (SetsTableRef.IsValid() == false)
	{
		SetsTableRef.LoadSynchronous();
	}
	WeaponList = SetsTableRef.Get();
}

const FWeaponSetsInfo* UWeaponDataSubsystem::GetWeaponInfo(const FName& WeaponName) const
{
	if (WeaponList)
	{
		return WeaponList->FindRow<FWeaponSetsInfo>(WeaponName, TEXT(""));
	}

	return nullptr;
}

bool UWeaponDataSubsystem::GetWeaponInfoBlueprint(const FName& WeaponName, FWeaponSetsInfo& OutWeaponInfo) const
{
	const FWeaponSetsInfo* FoundWeapon = GetWeaponInfo(WeaponName);
	if (!FoundWeapon)
	{
		return false;
	}

	OutWeaponInfo = *FoundWeapon;
	return true;
}
