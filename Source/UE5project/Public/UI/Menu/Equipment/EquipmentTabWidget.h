// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "Items/Armor/Data/ArmorData.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "Combat/Data/AttackData.h"
#include "EquipmentTabWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UWidget;
class UPanelWidget;
class UUniformGridPanel;
class UEquipmentComponent;
class UInventoryComponent;
class UPlayerStatComponent;
class UEquipmentGridEntryWidget;
class UStatRequirementRowWidget;
class UCompareStatRowWidget;
class APlayerBase;

UENUM(BlueprintType)
enum class EEquipmentTabCategory : uint8
{
	Weapon,
	Head,
	Chest,
	Hands,
	Legs
};

UCLASS(Abstract)
class UE5PROJECT_API UEquipmentTabWidget : public UBaseUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshEquipment();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnCategoryChanged(EEquipmentTabCategory NewCategory);

	// ---- 카테고리 탭 ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Category_Weapon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Category_Head;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Category_Chest;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Category_Hands;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Category_Legs;

	// ---- 아이템 그리드 ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UPanelWidget> Box_ItemGrid;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UEquipmentGridEntryWidget> GridEntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	int32 GridColumns = 5;

	// ---- 상세 패널: 헤더(이름+유형, 카테고리 라벨) ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_DetailContent;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_DetailItemName;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_WeaponTypeValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_DetailCategoryLabel;

	// ---- 상세 패널: 아이콘 + 스탯 2열 ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_DetailIcon;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_WeaponStatsRow;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_AttackPowerValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PoisePowerValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StancePowerValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_WeaponWeightValue;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_ArmorStatsRow;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ArmorDefenseValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ArmorMagicDefenseValue;

	// ---- 빈 선택 안내 ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_SelectionEmpty;

	// ---- 설명 ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_DetailDescription;

	// ---- 무기: 요구 스탯 (구분선 아래) ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_WeaponDetail;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UPanelWidget> Box_WeaponRequirements;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UStatRequirementRowWidget> StatRequirementRowWidgetClass;

	// ---- 방어구: 저항+무게 (구분선 아래) ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_ArmorDetail;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ArmorFireResistValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ArmorFrostResistValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ArmorPoisonResistValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ArmorBleedResistValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ArmorWeightValue;

	// ---- 비교 패널 ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_Compare;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Panel_CompareEmpty;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UPanelWidget> Box_CompareStats;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCompareStatRowWidget> CompareStatRowWidgetClass;

private:
	UFUNCTION() void OnCategoryWeaponClicked();
	UFUNCTION() void OnCategoryHeadClicked();
	UFUNCTION() void OnCategoryChestClicked();
	UFUNCTION() void OnCategoryHandsClicked();
	UFUNCTION() void OnCategoryLegsClicked();

	void SelectCategory(EEquipmentTabCategory Category);
	void SelectItem(FName ItemKey);
	void EquipItem(FName ItemKey);

	void RefreshGrid();
	void RefreshDetailPanel();
	void RefreshComparePanel();
	void RefreshWeaponGridGrouped(class UEquipmentComponent* Equip, class UInventoryComponent* Inventory);
	void RefreshArmorGridFlat(class UEquipmentComponent* Equip, class UInventoryComponent* Inventory);
	UUniformGridPanel* BuildGridSection(const TArray<FName>& Keys, FName EquippedKey);
	UWidget* CreateGridDivider() const;

	void HandleWeaponChanged(EWeaponType WeaponType);
	void HandleArmorChanged(EArmorSlot ArmorSlot);
	void HandleEntryClicked(FName ItemKey);
	void HandleEntryDoubleClicked(FName ItemKey);

	UEquipmentComponent* GetPlayerEquipment() const;
	UInventoryComponent* GetPlayerInventory() const;
	UPlayerStatComponent* GetPlayerStat() const;

	FAttackDamageSource EvaluateCandidateWeaponDamage(const FWeaponSetsInfo* Candidate) const;
	FWeaponRequirementBreakdown EvaluateCandidateWeaponRequirement(const FWeaponSetsInfo* Candidate) const;

	void AddCompareRow(const FText& Label, float CurrentValue, float NewValue);
	void SetDetailSectionsCollapsed();   // 5곳 반복되던 "전부 숨김" 로직 통합

	EArmorSlot CategoryToArmorSlot(EEquipmentTabCategory Category) const;
	FText CategoryToLabel(EEquipmentTabCategory Category) const;
	FText WeaponTypeToLabel(EWeaponType Type) const;
	UTexture2D* GetIconForKey(FName Key) const;

	EEquipmentTabCategory ActiveCategory = EEquipmentTabCategory::Weapon;
	FName SelectedItemKey = NAME_None;

	UPROPERTY()
	TArray<TObjectPtr<UEquipmentGridEntryWidget>> GridEntries;

	FDelegateHandle WeaponChangedHandle;
	FDelegateHandle ArmorChangedHandle;
};
