// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/Armor/Data/ArmorData.h"
#include "InventoryComponent.generated.h"

class UWeaponDataSubsystem;
class UArmorDataSubsystem;
class UItemDataSubsystem;

// 무기/방어구는 개별 인스턴스로 소유 (같은 아이템 여러 개 소유 가능, 스택 아님)
// InstanceId는 지금은 단순 식별용이지만, 나중에 강화/내구도 등 인스턴스별 데이터가 붙을 자리
USTRUCT(BlueprintType)
struct FOwnedEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemKey = NAME_None;
};

DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 드롭테이블/픽업/상점 등 모든 습득 경로가 호출하는 단일 진입점
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GrantItem(FName ItemKey, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveEquipmentInstance(FGuid InstanceId);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveStackableItem(FName ItemKey, int32 Quantity = 1);

	FORCEINLINE const TArray<FOwnedEquipmentEntry>& GetOwnedWeapons() const { return OwnedWeapons; }
	FORCEINLINE const TArray<FOwnedEquipmentEntry>& GetOwnedArmors() const { return OwnedArmors; }
	FORCEINLINE const TMap<FName, int32>& GetStackableItems() const { return StackableItems; }

	TArray<FName> GetOwnedWeaponKeys() const;
	TArray<FName> GetOwnedArmorKeysForSlot(EArmorSlot Slot) const;

	FOnInventoryChanged OnInventoryChanged;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<FOwnedEquipmentEntry> OwnedWeapons;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<FOwnedEquipmentEntry> OwnedArmors;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TMap<FName, int32> StackableItems;

	UWeaponDataSubsystem* GetWeaponSubsystem() const;
	UArmorDataSubsystem* GetArmorSubsystem() const;
	UItemDataSubsystem* GetItemSubsystem() const;
};
