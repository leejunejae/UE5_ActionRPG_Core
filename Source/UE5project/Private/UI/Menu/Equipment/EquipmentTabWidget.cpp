// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Menu/Equipment/EquipmentTabWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

#include "Characters/Player/PlayerBase.h"
#include "Characters/Components/EquipmentComponent.h"
#include "Characters/Player/Components/InventoryComponent.h"
#include "Characters/Player/Components/PlayerStatComponent.h"

#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Items/Armor/Data/ArmorDataAsset.h"

#include "Core/Subsystems/GameInstanceSystem/WeaponDataSubsystem.h"
#include "Core/Subsystems/GameInstanceSystem/ArmorDataSubsystem.h"

#include "UI/Menu/Equipment/EquipmentGridEntryWidget.h"
#include "UI/Menu/Equipment/StatRequirementRowWidget.h"
#include "UI/Menu/Equipment/CompareStatRowWidget.h"

void UEquipmentTabWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Category_Weapon) Btn_Category_Weapon->OnClicked.AddDynamic(this, &UEquipmentTabWidget::OnCategoryWeaponClicked);
	if (Btn_Category_Head)   Btn_Category_Head->OnClicked.AddDynamic(this, &UEquipmentTabWidget::OnCategoryHeadClicked);
	if (Btn_Category_Chest)  Btn_Category_Chest->OnClicked.AddDynamic(this, &UEquipmentTabWidget::OnCategoryChestClicked);
	if (Btn_Category_Hands)  Btn_Category_Hands->OnClicked.AddDynamic(this, &UEquipmentTabWidget::OnCategoryHandsClicked);
	if (Btn_Category_Legs)   Btn_Category_Legs->OnClicked.AddDynamic(this, &UEquipmentTabWidget::OnCategoryLegsClicked);

	if (UEquipmentComponent* Equip = GetPlayerEquipment())
	{
		WeaponChangedHandle = Equip->OnWeaponSetChanged().AddUObject(this, &UEquipmentTabWidget::HandleWeaponChanged);
		ArmorChangedHandle = Equip->OnArmorChangedDelegate.AddUObject(this, &UEquipmentTabWidget::HandleArmorChanged);
	}
}

void UEquipmentTabWidget::NativeDestruct()
{
	if (UEquipmentComponent* Equip = GetPlayerEquipment())
	{
		Equip->OnWeaponSetChanged().Remove(WeaponChangedHandle);
		Equip->OnArmorChangedDelegate.Remove(ArmorChangedHandle);
	}
	Super::NativeDestruct();
}

/* ============================================================
 *  참조 조회
 * ============================================================ */
UEquipmentComponent* UEquipmentTabWidget::GetPlayerEquipment() const
{
	APlayerController* PC = GetOwningPlayer();
	APlayerBase* Player = PC ? Cast<APlayerBase>(PC->GetPawn()) : nullptr;
	return Player ? Player->GetEquipmentComponent() : nullptr;
}

UInventoryComponent* UEquipmentTabWidget::GetPlayerInventory() const
{
	APlayerController* PC = GetOwningPlayer();
	APlayerBase* Player = PC ? Cast<APlayerBase>(PC->GetPawn()) : nullptr;
	return Player ? Player->FindComponentByClass<UInventoryComponent>() : nullptr;
}

UPlayerStatComponent* UEquipmentTabWidget::GetPlayerStat() const
{
	APlayerController* PC = GetOwningPlayer();
	APlayerBase* Player = PC ? Cast<APlayerBase>(PC->GetPawn()) : nullptr;
	return Player ? Cast<UPlayerStatComponent>(Player->GetStatComponent()) : nullptr;
}

/* ============================================================
 *  후보 무기 평가
 * ============================================================ */
FAttackDamageSource UEquipmentTabWidget::EvaluateCandidateWeaponDamage(const FWeaponSetsInfo* Candidate) const
{
	UPlayerStatComponent* PlayerStat = GetPlayerStat();
	if (!Candidate || !PlayerStat) return FAttackDamageSource();

	const float PerformanceRatio = PlayerStat->GetWeaponPerformanceRatio(Candidate->RequiredAttributes.ToCharacterStats());
	const FPlayerCombatStats& Combat = PlayerStat->GetCharacterStats().CombatStats;

	return CalculateWeaponAttackDamageSource(Candidate, PerformanceRatio,
		Combat.StrengthAttackBonus, Combat.DexterityAttackBonus, 0.f);
}

FWeaponRequirementBreakdown UEquipmentTabWidget::EvaluateCandidateWeaponRequirement(const FWeaponSetsInfo* Candidate) const
{
	UPlayerStatComponent* PlayerStat = GetPlayerStat();
	if (!Candidate || !PlayerStat) return FWeaponRequirementBreakdown();

	const FCharacterAttributes CurrentAttrs = PlayerStat->GetBaseAttributesLevel();
	const FPlayerCombatStats& Combat = PlayerStat->GetCharacterStats().CombatStats;

	return CalculateWeaponRequirementBreakdown(Candidate, CurrentAttrs,
		Combat.StrengthAttackBonus, Combat.DexterityAttackBonus, 0.f);
}

/* ============================================================
 *  라벨 / 헬퍼
 * ============================================================ */
EArmorSlot UEquipmentTabWidget::CategoryToArmorSlot(EEquipmentTabCategory Category) const
{
	switch (Category)
	{
	case EEquipmentTabCategory::Head:  return EArmorSlot::Head;
	case EEquipmentTabCategory::Chest: return EArmorSlot::Chest;
	case EEquipmentTabCategory::Hands: return EArmorSlot::Hands;
	case EEquipmentTabCategory::Legs:  return EArmorSlot::Legs;
	default: return EArmorSlot::Chest;
	}
}

FText UEquipmentTabWidget::CategoryToLabel(EEquipmentTabCategory Category) const
{
	switch (Category)
	{
	case EEquipmentTabCategory::Weapon: return FText::FromString(TEXT("무기"));
	case EEquipmentTabCategory::Head:   return FText::FromString(TEXT("머리"));
	case EEquipmentTabCategory::Chest:  return FText::FromString(TEXT("가슴"));
	case EEquipmentTabCategory::Hands:  return FText::FromString(TEXT("손"));
	case EEquipmentTabCategory::Legs:   return FText::FromString(TEXT("다리"));
	default: return FText::GetEmpty();
	}
}

FText UEquipmentTabWidget::WeaponTypeToLabel(EWeaponType Type) const
{
	switch (Type)
	{
	case EWeaponType::SwordAndShield: return FText::FromString(TEXT("한손검 & 방패"));
	case EWeaponType::LongSword:      return FText::FromString(TEXT("롱소드"));
	case EWeaponType::GreatSword:     return FText::FromString(TEXT("그레이트소드"));
	case EWeaponType::SpearAndShield: return FText::FromString(TEXT("창 & 방패"));
	case EWeaponType::Knuckles:       return FText::FromString(TEXT("너클"));
	default:                          return FText::FromString(TEXT("맨손"));
	}
}

UTexture2D* UEquipmentTabWidget::GetIconForKey(FName Key) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	if (ActiveCategory == EEquipmentTabCategory::Weapon)
	{
		if (UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>())
		{
			if (const FWeaponSetsInfo* Weapon = WeaponSubsystem->GetWeaponInfo(Key))
			{
				if (Weapon->WeaponDefenition.LoadSynchronous())
				{
					return Weapon->WeaponDefenition.Get()->WeaponInstance.SlotIcon;
				}
			}
		}
	}
	else
	{
		if (UArmorDataSubsystem* ArmorSubsystem = World->GetGameInstance()->GetSubsystem<UArmorDataSubsystem>())
		{
			if (const FArmorPieceInfo* Armor = ArmorSubsystem->GetArmorPieceInfo(Key))
			{
				if (Armor->ArmorDefinition.LoadSynchronous())
				{
					return Armor->ArmorDefinition.Get()->SlotIcon;
				}
			}
		}
	}
	return nullptr;
}

/* ============================================================
 *  Refresh 진입점
 * ============================================================ */
void UEquipmentTabWidget::RefreshEquipment()
{
	RefreshGrid();
	RefreshDetailPanel();
	RefreshComparePanel();
}

/* ============================================================
 *  카테고리 전환
 * ============================================================ */
void UEquipmentTabWidget::SelectCategory(EEquipmentTabCategory Category)
{
	ActiveCategory = Category;
	SelectedItemKey = NAME_None;
	OnCategoryChanged(Category);
	RefreshGrid();
	RefreshDetailPanel();
	RefreshComparePanel();
}

void UEquipmentTabWidget::OnCategoryWeaponClicked() { SelectCategory(EEquipmentTabCategory::Weapon); }
void UEquipmentTabWidget::OnCategoryHeadClicked() { SelectCategory(EEquipmentTabCategory::Head); }
void UEquipmentTabWidget::OnCategoryChestClicked() { SelectCategory(EEquipmentTabCategory::Chest); }
void UEquipmentTabWidget::OnCategoryHandsClicked() { SelectCategory(EEquipmentTabCategory::Hands); }
void UEquipmentTabWidget::OnCategoryLegsClicked() { SelectCategory(EEquipmentTabCategory::Legs); }

/* ============================================================
 *  그리드
 * ============================================================ */
void UEquipmentTabWidget::RefreshGrid()
{
	if (!Box_ItemGrid || !GridEntryWidgetClass) return;

	UEquipmentComponent* Equip = GetPlayerEquipment();
	UInventoryComponent* Inventory = GetPlayerInventory();
	if (!Equip || !Inventory) return;

	Box_ItemGrid->ClearChildren();
	GridEntries.Empty();

	if (ActiveCategory == EEquipmentTabCategory::Weapon)
	{
		RefreshWeaponGridGrouped(Equip, Inventory);
	}
	else
	{
		RefreshArmorGridFlat(Equip, Inventory);
	}
}

void UEquipmentTabWidget::RefreshWeaponGridGrouped(UEquipmentComponent* Equip, UInventoryComponent* Inventory)
{
	UWorld* World = GetWorld();
	UWeaponDataSubsystem* WeaponSubsystem = World ? World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>() : nullptr;
	if (!WeaponSubsystem) return;

	TArray<FName> AllKeys = Inventory->GetOwnedWeaponKeys();
	const FName EquippedKey = Equip->GetEquipedWeaponKey();

	if (SelectedItemKey == NAME_None && AllKeys.Num() > 0)
	{
		SelectedItemKey = AllKeys[0];
	}

	// 고정 정렬 순서 — 이 순서대로 그룹이 위에서부터 배치됨
	static const TArray<EWeaponType> TypeOrder = {
		EWeaponType::SwordAndShield,
		EWeaponType::LongSword,
		EWeaponType::GreatSword,
		EWeaponType::SpearAndShield,
		EWeaponType::Knuckles
	};

	TMap<EWeaponType, TArray<FName>> Grouped;
	for (const FName& Key : AllKeys)
	{
		const FWeaponSetsInfo* Info = WeaponSubsystem->GetWeaponInfo(Key);
		if (!Info || !Info->WeaponDefenition.LoadSynchronous()) continue;

		Grouped.FindOrAdd(Info->WeaponDefenition.Get()->WeaponType).Add(Key);
	}

	bool bAnyGroupRendered = false;

	for (const EWeaponType Type : TypeOrder)
	{
		TArray<FName>* GroupKeys = Grouped.Find(Type);
		if (!GroupKeys || GroupKeys->Num() == 0) continue;

		if (bAnyGroupRendered)
		{
			if (UVerticalBoxSlot* DividerSlot = Cast<UVerticalBoxSlot>(Box_ItemGrid->AddChild(CreateGridDivider())))
			{
				DividerSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 8.f));
			}
		}

		Box_ItemGrid->AddChild(BuildGridSection(*GroupKeys, EquippedKey));
		bAnyGroupRendered = true;
	}

	if (!bAnyGroupRendered)
	{
		Box_ItemGrid->AddChild(BuildGridSection(TArray<FName>(), EquippedKey));
	}
}

void UEquipmentTabWidget::RefreshArmorGridFlat(UEquipmentComponent* Equip, UInventoryComponent* Inventory)
{
	const EArmorSlot ArmorSlot = CategoryToArmorSlot(ActiveCategory);
	TArray<FName> Keys = Inventory->GetOwnedArmorKeysForSlot(ArmorSlot);
	const FName EquippedKey = Equip->GetEquipedArmorKey(ArmorSlot);

	if (SelectedItemKey == NAME_None && Keys.Num() > 0)
	{
		SelectedItemKey = Keys[0];
	}

	Box_ItemGrid->AddChild(BuildGridSection(Keys, EquippedKey));
}

UUniformGridPanel* UEquipmentTabWidget::BuildGridSection(const TArray<FName>& Keys, FName EquippedKey)
{
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	Grid->SetSlotPadding(FMargin(4.f, 4.f));

	const int32 RowCount = FMath::Max(1, FMath::DivideAndRoundUp(Keys.Num(), GridColumns));
	const int32 TotalCells = RowCount * GridColumns;

	for (int32 Index = 0; Index < TotalCells; ++Index)
	{
		UEquipmentGridEntryWidget* Entry = CreateWidget<UEquipmentGridEntryWidget>(this, GridEntryWidgetClass);
		if (!Entry) continue;

		if (Keys.IsValidIndex(Index))
		{
			const FName Key = Keys[Index];
			Entry->OnEntryClicked.BindUObject(this, &UEquipmentTabWidget::HandleEntryClicked);
			Entry->OnEntryDoubleClicked.BindUObject(this, &UEquipmentTabWidget::HandleEntryDoubleClicked);
			Entry->InitEntry(Key, GetIconForKey(Key), Key == EquippedKey, Key == SelectedItemKey);
		}
		else
		{
			Entry->InitEntry(NAME_None, nullptr, false, false);
		}

		Grid->AddChildToUniformGrid(Entry, Index / GridColumns, Index % GridColumns);
		GridEntries.Add(Entry);
	}

	return Grid;
}

UWidget* UEquipmentTabWidget::CreateGridDivider() const
{
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetHeightOverride(1.f);

	UBorder* Divider = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Divider->SetBrushColor(FLinearColor(0.3f, 0.27f, 0.18f, 0.5f));
	Box->AddChild(Divider);

	return Box;
}

void UEquipmentTabWidget::HandleEntryClicked(FName ItemKey) { SelectItem(ItemKey); }

void UEquipmentTabWidget::SelectItem(FName ItemKey)
{
	SelectedItemKey = ItemKey;
	for (UEquipmentGridEntryWidget* Entry : GridEntries)
	{
		if (Entry) Entry->SetSelected(Entry->GetItemKey() == ItemKey);
	}
	RefreshDetailPanel();
	RefreshComparePanel();
}

void UEquipmentTabWidget::HandleEntryDoubleClicked(FName ItemKey) { EquipItem(ItemKey); }

void UEquipmentTabWidget::EquipItem(FName ItemKey)
{
	UEquipmentComponent* Equip = GetPlayerEquipment();
	if (!Equip) return;

	if (ActiveCategory == EEquipmentTabCategory::Weapon)
	{
		Equip->EquipWeapon_Implementation(ItemKey);
	}
	else
	{
		Equip->EquipArmor(ItemKey);
	}
}

/* ============================================================
 *  상세 패널 — 전부 숨김 헬퍼 (5곳 반복되던 것 통합)
 * ============================================================ */
void UEquipmentTabWidget::SetDetailSectionsCollapsed()
{
	if (Panel_DetailContent)  Panel_DetailContent->SetVisibility(ESlateVisibility::Collapsed);
	if (Panel_SelectionEmpty) Panel_SelectionEmpty->SetVisibility(ESlateVisibility::Visible);
}

void UEquipmentTabWidget::RefreshDetailPanel()
{
	if (SelectedItemKey == NAME_None)
	{
		SetDetailSectionsCollapsed();
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	if (ActiveCategory == EEquipmentTabCategory::Weapon)
	{
		UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>();
		const FWeaponSetsInfo* Weapon = WeaponSubsystem ? WeaponSubsystem->GetWeaponInfo(SelectedItemKey) : nullptr;

		if (!Weapon || !Weapon->WeaponDefenition.LoadSynchronous())
		{
			SetDetailSectionsCollapsed();
			return;
		}

		if (Panel_DetailContent)  Panel_DetailContent->SetVisibility(ESlateVisibility::Visible);
		if (Panel_SelectionEmpty) Panel_SelectionEmpty->SetVisibility(ESlateVisibility::Collapsed);

		if (Panel_ArmorStatsRow) Panel_ArmorStatsRow->SetVisibility(ESlateVisibility::Collapsed);
		if (Panel_ArmorDetail)   Panel_ArmorDetail->SetVisibility(ESlateVisibility::Collapsed);
		if (Panel_WeaponStatsRow) Panel_WeaponStatsRow->SetVisibility(ESlateVisibility::Visible);
		if (Panel_WeaponDetail)   Panel_WeaponDetail->SetVisibility(ESlateVisibility::Visible);

		UWeaponDataAsset* WeaponAsset = Weapon->WeaponDefenition.Get();
		if (Image_DetailIcon) Image_DetailIcon->SetBrushFromTexture(WeaponAsset->WeaponInstance.Icon);
		if (Text_DetailItemName) Text_DetailItemName->SetText(WeaponAsset->DisplayName);
		if (Text_DetailCategoryLabel) Text_DetailCategoryLabel->SetText(CategoryToLabel(ActiveCategory));
		if (Text_DetailDescription) Text_DetailDescription->SetText(WeaponAsset->Description);

		if (Text_WeaponTypeValue)
		{
			Text_WeaponTypeValue->SetVisibility(ESlateVisibility::Visible);
			Text_WeaponTypeValue->SetText(WeaponTypeToLabel(WeaponAsset->WeaponType));
		}

		const FAttackDamageSource AtkSource = EvaluateCandidateWeaponDamage(Weapon);
		if (Text_AttackPowerValue) Text_AttackPowerValue->SetText(FText::AsNumber((int32)AtkSource.AttackRating));
		if (Text_PoisePowerValue)  Text_PoisePowerValue->SetText(FText::AsNumber((int32)AtkSource.PoiseRating));
		if (Text_StancePowerValue) Text_StancePowerValue->SetText(FText::AsNumber((int32)AtkSource.StanceRating));
		if (Text_WeaponWeightValue) Text_WeaponWeightValue->SetText(FText::AsNumber(Weapon->WeightValue));

		if (Box_WeaponRequirements && StatRequirementRowWidgetClass)
		{
			Box_WeaponRequirements->ClearChildren();
			const FWeaponRequirementBreakdown Breakdown = EvaluateCandidateWeaponRequirement(Weapon);

			auto AddRow = [this](const FText& Label, const FWeaponRequirementRow& Row)
				{
					if (UStatRequirementRowWidget* RowWidget = CreateWidget<UStatRequirementRowWidget>(this, StatRequirementRowWidgetClass))
					{
						RowWidget->InitRow(Label, Row);
						Box_WeaponRequirements->AddChild(RowWidget);
					}
				};

			AddRow(FText::FromString(TEXT("근력 (STR)")), Breakdown.Strength);
			AddRow(FText::FromString(TEXT("민첩 (DEX)")), Breakdown.Dexterity);
			AddRow(FText::FromString(TEXT("자연친화 (AFF)")), Breakdown.Affinity);
		}
	}
	else
	{
		UArmorDataSubsystem* ArmorSubsystem = World->GetGameInstance()->GetSubsystem<UArmorDataSubsystem>();
		const FArmorPieceInfo* Armor = ArmorSubsystem ? ArmorSubsystem->GetArmorPieceInfo(SelectedItemKey) : nullptr;

		if (!Armor || !Armor->ArmorDefinition.LoadSynchronous())
		{
			SetDetailSectionsCollapsed();
			return;
		}

		if (Panel_DetailContent)  Panel_DetailContent->SetVisibility(ESlateVisibility::Visible);
		if (Panel_SelectionEmpty) Panel_SelectionEmpty->SetVisibility(ESlateVisibility::Collapsed);

		if (Panel_WeaponStatsRow) Panel_WeaponStatsRow->SetVisibility(ESlateVisibility::Collapsed);
		if (Panel_WeaponDetail)   Panel_WeaponDetail->SetVisibility(ESlateVisibility::Collapsed);
		if (Text_WeaponTypeValue) Text_WeaponTypeValue->SetVisibility(ESlateVisibility::Collapsed);
		if (Panel_ArmorStatsRow)  Panel_ArmorStatsRow->SetVisibility(ESlateVisibility::Visible);
		if (Panel_ArmorDetail)    Panel_ArmorDetail->SetVisibility(ESlateVisibility::Visible);

		UArmorDataAsset* ArmorAsset = Armor->ArmorDefinition.Get();
		if (Image_DetailIcon) Image_DetailIcon->SetBrushFromTexture(ArmorAsset->Icon);
		if (Text_DetailItemName) Text_DetailItemName->SetText(ArmorAsset->DisplayName);
		if (Text_DetailCategoryLabel) Text_DetailCategoryLabel->SetText(CategoryToLabel(ActiveCategory));
		if (Text_DetailDescription) Text_DetailDescription->SetText(ArmorAsset->Description);

		if (Text_ArmorDefenseValue)      Text_ArmorDefenseValue->SetText(FText::AsNumber((int32)Armor->DefenseValue));
		if (Text_ArmorMagicDefenseValue) Text_ArmorMagicDefenseValue->SetText(FText::AsNumber((int32)Armor->MagicDefenseValue));
		if (Text_ArmorFireResistValue)   Text_ArmorFireResistValue->SetText(FText::AsNumber((int32)Armor->FireResistance));
		if (Text_ArmorFrostResistValue)  Text_ArmorFrostResistValue->SetText(FText::AsNumber((int32)Armor->FrostResistance));
		if (Text_ArmorPoisonResistValue) Text_ArmorPoisonResistValue->SetText(FText::AsNumber((int32)Armor->PoisonResistance));
		if (Text_ArmorBleedResistValue)  Text_ArmorBleedResistValue->SetText(FText::AsNumber((int32)Armor->BleedResistance));
		if (Text_ArmorWeightValue)       Text_ArmorWeightValue->SetText(FText::AsNumber(Armor->WeightValue));
	}
}

/* ============================================================
 *  비교 패널
 * ============================================================ */
void UEquipmentTabWidget::AddCompareRow(const FText& Label, float CurrentValue, float NewValue)
{
	if (!Box_CompareStats || !CompareStatRowWidgetClass) return;

	if (UCompareStatRowWidget* RowWidget = CreateWidget<UCompareStatRowWidget>(this, CompareStatRowWidgetClass))
	{
		RowWidget->InitRow(Label, CurrentValue, NewValue);
		Box_CompareStats->AddChild(RowWidget);
	}
}

void UEquipmentTabWidget::RefreshComparePanel()
{
	UEquipmentComponent* Equip = GetPlayerEquipment();
	UWorld* World = GetWorld();
	if (!Equip || !World) return;

	const FName EquippedKey = (ActiveCategory == EEquipmentTabCategory::Weapon)
		? Equip->GetEquipedWeaponKey()
		: Equip->GetEquipedArmorKey(CategoryToArmorSlot(ActiveCategory));

	if (SelectedItemKey == NAME_None || SelectedItemKey == EquippedKey)
	{
		if (Panel_Compare)      Panel_Compare->SetVisibility(ESlateVisibility::Collapsed);
		if (Panel_CompareEmpty) Panel_CompareEmpty->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	if (Panel_CompareEmpty) Panel_CompareEmpty->SetVisibility(ESlateVisibility::Collapsed);
	if (Panel_Compare)      Panel_Compare->SetVisibility(ESlateVisibility::Visible);

	if (Box_CompareStats) Box_CompareStats->ClearChildren();

	if (ActiveCategory == EEquipmentTabCategory::Weapon)
	{
		UWeaponDataSubsystem* WeaponSubsystem = World->GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>();
		const FWeaponSetsInfo* CurrentWeapon = Equip->GetEquipedWeapon();
		const FWeaponSetsInfo* NewWeapon = WeaponSubsystem ? WeaponSubsystem->GetWeaponInfo(SelectedItemKey) : nullptr;

		const FAttackDamageSource CurrentSource = Equip->GetAttackDamageSource();
		const FAttackDamageSource NewSource = EvaluateCandidateWeaponDamage(NewWeapon);

		AddCompareRow(FText::FromString(TEXT("공격력")), CurrentSource.AttackRating, NewSource.AttackRating);
		AddCompareRow(FText::FromString(TEXT("강인도")), CurrentSource.PoiseRating, NewSource.PoiseRating);
		AddCompareRow(FText::FromString(TEXT("자세")), CurrentSource.StanceRating, NewSource.StanceRating);
		AddCompareRow(FText::FromString(TEXT("무게")), CurrentWeapon ? CurrentWeapon->WeightValue : 0.f, NewWeapon ? NewWeapon->WeightValue : 0.f);
	}
	else
	{
		UArmorDataSubsystem* ArmorSubsystem = World->GetGameInstance()->GetSubsystem<UArmorDataSubsystem>();
		const FArmorPieceInfo* CurrentArmor = Equip->GetEquipedArmor(CategoryToArmorSlot(ActiveCategory));
		const FArmorPieceInfo* NewArmor = ArmorSubsystem ? ArmorSubsystem->GetArmorPieceInfo(SelectedItemKey) : nullptr;

		AddCompareRow(FText::FromString(TEXT("물리 방어")), CurrentArmor ? CurrentArmor->DefenseValue : 0.f, NewArmor ? NewArmor->DefenseValue : 0.f);
		AddCompareRow(FText::FromString(TEXT("마법 방어")), CurrentArmor ? CurrentArmor->MagicDefenseValue : 0.f, NewArmor ? NewArmor->MagicDefenseValue : 0.f);
		AddCompareRow(FText::FromString(TEXT("화상 저항")), CurrentArmor ? CurrentArmor->FireResistance : 0.f, NewArmor ? NewArmor->FireResistance : 0.f);
		AddCompareRow(FText::FromString(TEXT("동상 저항")), CurrentArmor ? CurrentArmor->FrostResistance : 0.f, NewArmor ? NewArmor->FrostResistance : 0.f);
		AddCompareRow(FText::FromString(TEXT("중독 저항")), CurrentArmor ? CurrentArmor->PoisonResistance : 0.f, NewArmor ? NewArmor->PoisonResistance : 0.f);
		AddCompareRow(FText::FromString(TEXT("출혈 저항")), CurrentArmor ? CurrentArmor->BleedResistance : 0.f, NewArmor ? NewArmor->BleedResistance : 0.f);
		AddCompareRow(FText::FromString(TEXT("무게")), CurrentArmor ? CurrentArmor->WeightValue : 0.f, NewArmor ? NewArmor->WeightValue : 0.f);
	}
}

/* ============================================================
 *  장착 변경 델리게이트
 * ============================================================ */
void UEquipmentTabWidget::HandleWeaponChanged(EWeaponType WeaponType)
{
	if (ActiveCategory == EEquipmentTabCategory::Weapon)
	{
		RefreshGrid();
		RefreshDetailPanel();
		RefreshComparePanel();
	}
}

void UEquipmentTabWidget::HandleArmorChanged(EArmorSlot ArmorSlot)
{
	if (ActiveCategory != EEquipmentTabCategory::Weapon && CategoryToArmorSlot(ActiveCategory) == ArmorSlot)
	{
		RefreshGrid();
		RefreshDetailPanel();
		RefreshComparePanel();
	}
}