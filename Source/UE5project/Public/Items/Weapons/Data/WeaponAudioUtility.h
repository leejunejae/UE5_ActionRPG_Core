#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Data/WeaponAudioData.h"

class USoundBase;
class UWeaponDataAsset;
class UWeaponDataSubsystem;

namespace WeaponAudioUtility
{
	UE5PROJECT_API void LoadWeaponSounds(
		const UWeaponDataAsset* WeaponData,
		const UWeaponDataSubsystem* WeaponSubsystem,
		TMap<FGameplayTag, FLoadedWeaponSoundSet>& OutSounds);

	UE5PROJECT_API USoundBase* SelectRandomWeaponSound(
		const TMap<FGameplayTag, FLoadedWeaponSoundSet>& Sounds,
		FGameplayTag WeaponSoundTag);
}
