// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Items/Weapons/Data/WeaponDataAsset.h"

void UWeaponDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

	TSoftObjectPtr<UDataTable> SetsTableRef = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/02_Item/Equipment/Weapon/Data/PlayerWeaponData_DT.PlayerWeaponData_DT")));
	if (SetsTableRef.IsValid() == false)
	{
		SetsTableRef.LoadSynchronous();
	}
	WeaponList = SetsTableRef.Get();

	TSoftObjectPtr<UWeaponAudioProfileSet> AudioProfilesRef(
		FSoftObjectPath(TEXT("/Game/06_Sound/AttackSound/WeaponSwingSound_DA.WeaponSwingSound_DA")));
	DefaultWeaponAudioProfiles = AudioProfilesRef.LoadSynchronous();
	WeaponSoundAttenuation = DefaultWeaponAudioProfiles
		? DefaultWeaponAudioProfiles->AttenuationSettings.LoadSynchronous()
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
	const UWeaponDataAsset* WeaponData) const
{
	if (!WeaponData) return nullptr;

	const TSoftObjectPtr<UWeaponAudioProfile>& Override =
		WeaponData->WeaponInstance.WeaponAudioProfileOverride;
	if (!Override.IsNull()) return Override;

	if (!DefaultWeaponAudioProfiles) return nullptr;

	const TSoftObjectPtr<UWeaponAudioProfile> CategoryProfile =
		DefaultWeaponAudioProfiles->FindProfile(WeaponData->GetEffectiveWeaponCategory());
	return !CategoryProfile.IsNull()
		? CategoryProfile
		: DefaultWeaponAudioProfiles->FindLegacyProfile(WeaponData->WeaponType);
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
