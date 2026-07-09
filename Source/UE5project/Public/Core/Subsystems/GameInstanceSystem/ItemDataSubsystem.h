// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/Misc/Data/MiscItemData.h"
#include "ItemDataSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UItemDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	TObjectPtr<UDataTable> ItemList = nullptr;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FMiscItemInfo* GetItemInfo(const FName& ItemKey) const;

	UFUNCTION(BlueprintCallable)
	bool GetItemInfoBlueprint(const FName& ItemKey, FMiscItemInfo& OutInfo) const;
};
