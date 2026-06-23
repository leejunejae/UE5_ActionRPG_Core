// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/Armor/Data/ArmorData.h"
#include "ArmorDataSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UArmorDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	TObjectPtr<UDataTable> ArmorPieceList = nullptr;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FArmorPieceInfo* GetArmorPieceInfo(const FName& ArmorKey) const;

	UFUNCTION(BlueprintCallable)
	bool GetArmorPieceInfoBlueprint(const FName& ArmorKey, FArmorPieceInfo& OutInfo) const;
};
