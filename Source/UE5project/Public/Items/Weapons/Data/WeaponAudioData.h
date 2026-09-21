#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "WeaponAudioData.generated.h"

class USoundBase;
class USoundAttenuation;

/** 같은 동작이 반복될 때 단조롭지 않도록 무작위 선택할 사운드 변형. */
USTRUCT(BlueprintType)
struct FWeaponSoundSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Audio")
	TArray<TSoftObjectPtr<USoundBase>> Variations;
};

/** 장비 시 미리 로드한 런타임 사운드 집합. */
USTRUCT()
struct FLoadedWeaponSoundSet
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> Variations;
};

/** 한 무기 또는 무기 유형에서 사용하는 동작 사운드 모음. */
UCLASS(BlueprintType)
class UE5PROJECT_API UWeaponAudioProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Audio",
		meta = (Categories = "Audio.Weapon"))
	TMap<FGameplayTag, FWeaponSoundSet> WeaponSounds;
};

/** 무기 유형별 기본 오디오 프로필과 공통 공간 감쇠 설정. */
UCLASS(BlueprintType)
class UE5PROJECT_API UWeaponAudioProfileSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Audio")
	TMap<EWeaponCategory, TSoftObjectPtr<UWeaponAudioProfile>> CategoryProfiles;

	/** 모든 무기 동작 사운드에 공통으로 적용할 3D 감쇠 설정. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Audio")
	TSoftObjectPtr<USoundAttenuation> AttenuationSettings;

	TSoftObjectPtr<UWeaponAudioProfile> FindProfile(EWeaponCategory WeaponCategory) const
	{
		const TSoftObjectPtr<UWeaponAudioProfile>* Found = CategoryProfiles.Find(WeaponCategory);
		return Found ? *Found : TSoftObjectPtr<UWeaponAudioProfile>();
	}
};
