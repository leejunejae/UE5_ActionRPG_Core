// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/Components/InventoryComponent.h"
#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/ArmorDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/ItemDataSubsystem.h"
#include "Items/Armor/Data/ArmorDataAsset.h"
#include "Utils/CoreLog.h"

UWeaponDataSubsystem* UInventoryComponent::GetWeaponSubsystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>() : nullptr;
}

UArmorDataSubsystem* UInventoryComponent::GetArmorSubsystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetGameInstance()->GetSubsystem<UArmorDataSubsystem>() : nullptr;
}

UItemDataSubsystem* UInventoryComponent::GetItemSubsystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetGameInstance()->GetSubsystem<UItemDataSubsystem>() : nullptr;
}

bool UInventoryComponent::GrantItem(FName ItemKey, int32 Quantity)
{
	if (ItemKey == NAME_None || Quantity <= 0) return false;

	if (UWeaponDataSubsystem* WeaponSubsystem = GetWeaponSubsystem())
	{
		if (WeaponSubsystem->GetWeaponInfo(ItemKey))
		{
			for (int32 i = 0; i < Quantity; ++i)
			{
				OwnedWeapons.Add(FOwnedEquipmentEntry{ FGuid::NewGuid(), ItemKey });
			}
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	if (UArmorDataSubsystem* ArmorSubsystem = GetArmorSubsystem())
	{
		if (ArmorSubsystem->GetArmorPieceInfo(ItemKey))
		{
			for (int32 i = 0; i < Quantity; ++i)
			{
				OwnedArmors.Add(FOwnedEquipmentEntry{ FGuid::NewGuid(), ItemKey });
			}
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	if (UItemDataSubsystem* ItemSubsystem = GetItemSubsystem())
	{
		if (ItemSubsystem->GetItemInfo(ItemKey))
		{
			StackableItems.FindOrAdd(ItemKey) += Quantity;
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[InventoryComponent] GrantItem: 알 수 없는 ItemKey (%s)"), *ItemKey.ToString());
	return false;
}

bool UInventoryComponent::RemoveEquipmentInstance(FGuid InstanceId)
{
	int32 Removed = OwnedWeapons.RemoveAll([InstanceId](const FOwnedEquipmentEntry& E) { return E.InstanceId == InstanceId; });
	Removed += OwnedArmors.RemoveAll([InstanceId](const FOwnedEquipmentEntry& E) { return E.InstanceId == InstanceId; });

	if (Removed > 0) OnInventoryChanged.Broadcast();
	return Removed > 0;
}

bool UInventoryComponent::RemoveStackableItem(FName ItemKey, int32 Quantity)
{
	int32* Current = StackableItems.Find(ItemKey);
	if (!Current || *Current < Quantity) return false;

	*Current -= Quantity;
	if (*Current <= 0) StackableItems.Remove(ItemKey);

	OnInventoryChanged.Broadcast();
	return true;
}

TArray<FName> UInventoryComponent::GetOwnedWeaponKeys() const
{
	TArray<FName> Keys;
	Keys.Reserve(OwnedWeapons.Num());
	for (const FOwnedEquipmentEntry& Entry : OwnedWeapons) Keys.Add(Entry.ItemKey);
	return Keys;
}

TArray<FName> UInventoryComponent::GetOwnedArmorKeysForSlot(EArmorSlot Slot) const
{
	TArray<FName> Result;

	UArmorDataSubsystem* ArmorSubsystem = GetArmorSubsystem();
	if (!ArmorSubsystem) return Result;

	for (const FOwnedEquipmentEntry& Entry : OwnedArmors)
	{
		const FArmorPieceInfo* Info = ArmorSubsystem->GetArmorPieceInfo(Entry.ItemKey);
		if (!Info) continue;

		UArmorDataAsset* Asset = Info->ArmorDefinition.LoadSynchronous();
		if (Asset && Asset->ArmorSlot == Slot)
		{
			Result.Add(Entry.ItemKey);
		}
	}

	return Result;
}