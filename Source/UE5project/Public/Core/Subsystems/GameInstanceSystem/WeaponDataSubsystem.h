// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/Weapons/Data/WeaponData.h"
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
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FWeaponSetsInfo* GetWeaponInfo(const FName& WeaponName) const;

	UFUNCTION(BlueprintCallable)
		bool GetWeaponInfoBlueprint(const FName& WeaponName, FWeaponSetsInfo& OutWeaponInfo) const;
};
