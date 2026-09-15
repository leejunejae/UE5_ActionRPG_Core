#include "Items/Weapons/Data/WeaponAudioUtility.h"

#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Sound/SoundBase.h"

void WeaponAudioUtility::LoadWeaponSounds(
	const UWeaponDataAsset* WeaponData,
	const UWeaponDataSubsystem* WeaponSubsystem,
	TMap<FGameplayTag, FLoadedWeaponSoundSet>& OutSounds)
{
	OutSounds.Reset();
	if (!WeaponData || !WeaponSubsystem) return;

	const TSoftObjectPtr<UWeaponAudioProfile> ProfileRef =
		WeaponSubsystem->ResolveWeaponAudioProfile(WeaponData);
	const UWeaponAudioProfile* Profile = ProfileRef.LoadSynchronous();
	if (!Profile) return;

	const FGameplayTag WeaponAudioRoot = FGameplayTag::RequestGameplayTag(TEXT("Audio.Weapon"), false);
	for (const TPair<FGameplayTag, FWeaponSoundSet>& Pair : Profile->WeaponSounds)
	{
		if (!WeaponAudioRoot.IsValid() || !Pair.Key.MatchesTag(WeaponAudioRoot))
		{
			UE_LOG(LogTemp, Warning, TEXT("Weapon audio profile contains a tag outside Audio.Weapon: %s"),
				*Pair.Key.ToString());
			continue;
		}

		FLoadedWeaponSoundSet LoadedSet;
		LoadedSet.Variations.Reserve(Pair.Value.Variations.Num());
		for (const TSoftObjectPtr<USoundBase>& SoundRef : Pair.Value.Variations)
		{
			if (USoundBase* Sound = SoundRef.LoadSynchronous())
			{
				LoadedSet.Variations.Add(Sound);
			}
		}
		if (!LoadedSet.Variations.IsEmpty())
		{
			OutSounds.Add(Pair.Key, MoveTemp(LoadedSet));
		}
	}
}

USoundBase* WeaponAudioUtility::SelectRandomWeaponSound(
	const TMap<FGameplayTag, FLoadedWeaponSoundSet>& Sounds,
	FGameplayTag WeaponSoundTag)
{
	if (!WeaponSoundTag.IsValid()) return nullptr;
	const FLoadedWeaponSoundSet* Found = Sounds.Find(WeaponSoundTag);
	if (!Found || Found->Variations.IsEmpty()) return nullptr;
	return Found->Variations[FMath::RandRange(0, Found->Variations.Num() - 1)];
}
