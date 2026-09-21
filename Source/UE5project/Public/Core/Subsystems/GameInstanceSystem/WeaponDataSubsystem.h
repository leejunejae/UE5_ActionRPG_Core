// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "Items/Weapons/Data/WeaponAudioData.h"
#include "Engine/DataTable.h"
#include "WeaponDataSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UWeaponDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
private:
	UPROPERTY(EditDefaultsOnly)
		TObjectPtr<UDataTable> WeaponList = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> OffHandWeaponList = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWeaponAudioProfileSet> DefaultWeaponAudioProfiles = nullptr;

	/** DefaultWeaponAudioProfiles에서 시작 시 한 번 로드해 유지하는 공통 감쇠 설정. */
	UPROPERTY(Transient)
	TObjectPtr<class USoundAttenuation> WeaponSoundAttenuation = nullptr;
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FWeaponSetsInfo* GetWeaponInfo(const FName& WeaponName) const;
	const FOffHandWeaponSetsInfo* GetOffHandWeaponInfo(const FName& WeaponName) const;

	TSoftObjectPtr<UWeaponAudioProfile> ResolveWeaponAudioProfile(
		const class UWeaponEquipmentDataAsset* WeaponData) const;

	class USoundAttenuation* GetWeaponSoundAttenuation() const
	{
		return WeaponSoundAttenuation;
	}

	UFUNCTION(BlueprintCallable)
		bool GetWeaponInfoBlueprint(const FName& WeaponName, FWeaponSetsInfo& OutWeaponInfo) const;

	UFUNCTION(BlueprintCallable)
	bool GetOffHandWeaponInfoBlueprint(const FName& WeaponName, FOffHandWeaponSetsInfo& OutWeaponInfo) const;
};
