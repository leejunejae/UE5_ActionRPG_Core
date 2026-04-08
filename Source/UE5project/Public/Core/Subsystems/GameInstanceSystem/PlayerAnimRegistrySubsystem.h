// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Animation/Data/PlayerAnimSetDataAsset.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerAnimRegistrySubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UPlayerAnimRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly)
		TObjectPtr<UPlayerAnimSetDataAsset> PlayerAnimAsset;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	const FPlayerAnimSet* GetPlayerAnimSet(const EWeaponType& WeaponType) const;
};
