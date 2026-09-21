// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Utils/CoreLog.h"

void UWeaponDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

	TSoftObjectPtr<UDataTable> SetsTableRef = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/02_Item/Equipment/Weapon/Data/PlayerWeaponData_DT.PlayerWeaponData_DT")));
	WeaponList = SetsTableRef.LoadSynchronous();
	if (!WeaponList)
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("Failed to load the main-hand weapon data table."));
	}

	TSoftObjectPtr<UDataTable> OffHandTableRef(FSoftObjectPath(
		TEXT("/Game/02_Item/Equipment/Weapon/Data/PlayerOffHandWeaponData_DT.PlayerOffHandWeaponData_DT")));
	OffHandWeaponList = OffHandTableRef.LoadSynchronous();
	if (!OffHandWeaponList)
	{
		UE_LOG(Log_Equip_Weapon, Error, TEXT("Failed to load the off-hand weapon data table."));
	}

	TSoftObjectPtr<UWeaponAudioProfileSet> AudioProfilesRef(
		FSoftObjectPath(TEXT("/Game/06_Sound/AttackSound/WeaponSwingSound_DA.WeaponSwingSound_DA")));
	DefaultWeaponAudioProfiles = AudioProfilesRef.LoadSynchronous();
	if (!DefaultWeaponAudioProfiles)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("Failed to load the default weapon audio profile set."));
	}
	WeaponSoundAttenuation = DefaultWeaponAudioProfiles
		? DefaultWeaponAudioProfiles->AttenuationSettings.LoadSynchronous()
		: nullptr;
	if (DefaultWeaponAudioProfiles && !DefaultWeaponAudioProfiles->AttenuationSettings.IsNull() && !WeaponSoundAttenuation)
	{
		UE_LOG(Log_Equip_Weapon, Warning, TEXT("Failed to load the weapon sound attenuation settings."));
	}
}

const FOffHandWeaponSetsInfo* UWeaponDataSubsystem::GetOffHandWeaponInfo(const FName& WeaponName) const
{
	return OffHandWeaponList
		? OffHandWeaponList->FindRow<FOffHandWeaponSetsInfo>(WeaponName, TEXT(""))
		: nullptr;
}

const FWeaponSetsInfo* UWeaponDataSubsystem::GetWeaponInfo(const FName& WeaponName) const
{
	if (WeaponList)
	{
		return WeaponList->FindRow<FWeaponSetsInfo>(WeaponName, TEXT(""));
	}

	return nullptr;
}

TSoftObjectPtr<UWeaponAudioProfile> UWeaponDataSubsystem::ResolveWeaponAudioProfile(
	const UWeaponEquipmentDataAsset* WeaponData) const
{
	if (!WeaponData) return nullptr;

	const TSoftObjectPtr<UWeaponAudioProfile>& Override =
		WeaponData->WeaponInstance.WeaponAudioProfileOverride;
	if (!Override.IsNull()) return Override;

	if (!DefaultWeaponAudioProfiles) return nullptr;

	return DefaultWeaponAudioProfiles->FindProfile(WeaponData->GetEffectiveWeaponCategory());
}

bool UWeaponDataSubsystem::GetOffHandWeaponInfoBlueprint(
	const FName& WeaponName, FOffHandWeaponSetsInfo& OutWeaponInfo) const
{
	const FOffHandWeaponSetsInfo* FoundWeapon = GetOffHandWeaponInfo(WeaponName);
	if (!FoundWeapon) return false;
	OutWeaponInfo = *FoundWeapon;
	return true;
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
