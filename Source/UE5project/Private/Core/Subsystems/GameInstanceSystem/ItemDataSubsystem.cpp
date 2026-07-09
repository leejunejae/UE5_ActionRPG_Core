// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/ItemDataSubsystem.h"

void UItemDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TSoftObjectPtr<UDataTable> TableRef = TSoftObjectPtr<UDataTable>(
		FSoftObjectPath(TEXT("/Game/02_Item/Misc/Data/MiscItemData_DT.MiscItemData_DT")));

	if (!TableRef.IsValid())
	{
		TableRef.LoadSynchronous();
	}
	ItemList = TableRef.Get();
}

const FMiscItemInfo* UItemDataSubsystem::GetItemInfo(const FName& ItemKey) const
{
	if (ItemList)
	{
		return ItemList->FindRow<FMiscItemInfo>(ItemKey, TEXT(""));
	}
	return nullptr;
}

bool UItemDataSubsystem::GetItemInfoBlueprint(const FName& ItemKey, FMiscItemInfo& OutInfo) const
{
	const FMiscItemInfo* Found = GetItemInfo(ItemKey);
	if (!Found) return false;
	OutInfo = *Found;
	return true;
}