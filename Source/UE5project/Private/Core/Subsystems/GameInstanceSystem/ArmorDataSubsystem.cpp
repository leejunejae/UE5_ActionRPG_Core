// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/ArmorDataSubsystem.h"

void UArmorDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TSoftObjectPtr<UDataTable> TableRef = TSoftObjectPtr<UDataTable>(
		FSoftObjectPath(TEXT("/Game/02_Item/Equipment/Armor/Data/PlayerArmorData_DT.PlayerArmorData_DT")));

	if (!TableRef.IsValid())
	{
		TableRef.LoadSynchronous();
	}
	ArmorPieceList = TableRef.Get();
}

const FArmorPieceInfo* UArmorDataSubsystem::GetArmorPieceInfo(const FName& ArmorKey) const
{
	if (ArmorPieceList)
	{
		return ArmorPieceList->FindRow<FArmorPieceInfo>(ArmorKey, TEXT(""));
	}
	return nullptr;
}

bool UArmorDataSubsystem::GetArmorPieceInfoBlueprint(const FName& ArmorKey, FArmorPieceInfo& OutInfo) const
{
	const FArmorPieceInfo* Found = GetArmorPieceInfo(ArmorKey);
	if (!Found) return false;
	OutInfo = *Found;
	return true;
}