// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/StatusTabWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Characters/Components/EquipmentComponent.h"

#include "Components/WidgetSwitcher.h"
#include "Characters/Preview/CharacterPreviewActor.h"
#include "Characters/Player/Controller/ControllerBase.h"

void UStatusTabWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Vitality_Plus)   Btn_Vitality_Plus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnVitalityPlusClicked);
    if (Btn_Vitality_Minus)  Btn_Vitality_Minus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnVitalityMinusClicked);
    if (Btn_Endurance_Plus)  Btn_Endurance_Plus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnEnduranceePlusClicked);
    if (Btn_Endurance_Minus) Btn_Endurance_Minus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnEnduranceMinusClicked);
    if (Btn_Mentality_Plus)  Btn_Mentality_Plus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnMentalityPlusClicked);
    if (Btn_Mentality_Minus) Btn_Mentality_Minus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnMentalityMinusClicked);
    if (Btn_Strength_Plus)   Btn_Strength_Plus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnStrengthPlusClicked);
    if (Btn_Strength_Minus)  Btn_Strength_Minus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnStrengthMinusClicked);
    if (Btn_Dexterity_Plus)  Btn_Dexterity_Plus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnDexterityPlusClicked);
    if (Btn_Dexterity_Minus) Btn_Dexterity_Minus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnDexterityMinusClicked);
    if (Btn_Affinity_Plus)   Btn_Affinity_Plus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnAffinityPlusClicked);
    if (Btn_Affinity_Minus)  Btn_Affinity_Minus->OnClicked.AddDynamic(this, &UStatusTabWidget::OnAffinityMinusClicked);
    if (Btn_ConfirmAllocation) Btn_ConfirmAllocation->OnClicked.AddDynamic(this, &UStatusTabWidget::OnConfirmClicked);
}

void UStatusTabWidget::InitializeWithPlayer(APlayerBase* InPlayer)
{
    if (!InPlayer) return;

    RefreshStats();
}

void UStatusTabWidget::RefreshStats()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    APlayerBase* Player = Cast<APlayerBase>(PC->GetPawn());
    if (!Player) return;

    UPlayerStatComponent* StatComp = Cast<UPlayerStatComponent>(Player->GetStatComponent());
    if (!StatComp) return;

    UEquipmentComponent* EquipComp = Player->FindComponentByClass<UEquipmentComponent>();


    const FPlayerStats Stats = StatComp->GetCharacterStats_Native();
    const FCharacterAttributes Attrs = StatComp->GetBaseAttributesLevel_Implementation();
    const FCharacterStats& Base = Stats.BaseStats;
    const FPlayerCombatStats& Combat = Stats.CombatStats;

    // ---- 능력치 ----
    if (Text_HPValue)
        Text_HPValue->SetText(FText::FromString(
            FString::Printf(TEXT("%.0f / %.0f"), Base.GetHealth(), Base.GetMaxHealth())));
    if (ProgressBar_HP)
        ProgressBar_HP->SetPercent(Base.GetHealthPercent());

    if (Text_SPValue)
        Text_SPValue->SetText(FText::FromString(
            FString::Printf(TEXT("%.0f / %.0f"), Stats.Stamina.Current, Stats.Stamina.Max)));
    if (ProgressBar_SP)
        ProgressBar_SP->SetPercent(Stats.Stamina.Max > 0.f ? Stats.Stamina.Current / Stats.Stamina.Max : 0.f);

    if (Text_FPValue)
        Text_FPValue->SetText(FText::FromString(
            FString::Printf(TEXT("%.0f / %.0f"), Stats.Focus.Current, Stats.Focus.Max)));
    if (ProgressBar_FP)
        ProgressBar_FP->SetPercent(Stats.Focus.Max > 0.f ? Stats.Focus.Current / Stats.Focus.Max : 0.f);

    if (Text_EquipLoadValue)
        Text_EquipLoadValue->SetText(FText::FromString(
            FString::Printf(TEXT("%.1f / %.1f"), Stats.EquipLoad.Current, Stats.EquipLoad.Max)));
    if (ProgressBar_EquipLoad)
        ProgressBar_EquipLoad->SetPercent(Stats.EquipLoad.Max > 0.f ? Stats.EquipLoad.Current / Stats.EquipLoad.Max : 0.f);

    // 스태미나 회복: 기본 + 민첩 보너스
    if (Text_SPRegenValue)
    {
        const float BaseRegen = StatComp->GetStaminaRegenRate();
        const float Bonus = Combat.StaminaRegenBonus;
        if (Bonus > 0.f)
            Text_SPRegenValue->SetText(FText::FromString(
                FString::Printf(TEXT("%.0f +%.0f / 초"), BaseRegen, Bonus)));
        else
            Text_SPRegenValue->SetText(FText::FromString(
                FString::Printf(TEXT("%.0f / 초"), BaseRegen)));
    }

    // ---- 특성 ----
    auto SetAttr = [](UTextBlock* TB, int32 Val)
        {
            if (TB) TB->SetText(FText::AsNumber(Val));
        };
    SetAttr(Text_VitalityValue, Attrs.Vitality);
    SetAttr(Text_EnduranceValue, Attrs.Endurance);
    SetAttr(Text_MentalityValue, Attrs.Mentality);
    SetAttr(Text_StrengthValue, Attrs.Strength);
    SetAttr(Text_DexterityValue, Attrs.Dexterity);
    SetAttr(Text_AffinityValue, Attrs.Affinity);

    // ---- 방어 ----
    if (Text_PhysicalDefenseValue)
        Text_PhysicalDefenseValue->SetText(FText::AsNumber((int32)Base.PhysicalDefense));
    if (Text_MagicDefenseValue)
        Text_MagicDefenseValue->SetText(FText::AsNumber((int32)Base.MagicDefense));
    if (Text_FireResistanceValue)
        Text_FireResistanceValue->SetText(FText::AsNumber((int32)Base.FireResistance));
    if (Text_FrostResistanceValue)
        Text_FrostResistanceValue->SetText(FText::AsNumber((int32)Base.FrostResistance));
    if (Text_PoisonResistanceValue)
        Text_PoisonResistanceValue->SetText(FText::AsNumber((int32)Base.PoisonResistance));
    if (Text_BleedResistanceValue)
        Text_BleedResistanceValue->SetText(FText::AsNumber((int32)Base.BleedResistance));
    if (Text_PoiseValue)
        Text_PoiseValue->SetText(FText::AsNumber((int32)Base.Poise.Max));

    // ---- 공격력: EquipmentComponent에서 계산된 값 사용 ----
    const FAttackDamageSource AtkSource = EquipComp->GetAttackDamageSource();
    if (Text_PhysicalAttackPowerValue)
        Text_PhysicalAttackPowerValue->SetText(FText::AsNumber((int32)AtkSource.AttackRating));
    if (Text_PoiseAttackPowerValue)
        Text_PoiseAttackPowerValue->SetText(FText::AsNumber((int32)AtkSource.PoiseRating));
    if (Text_StanceAttackPowerValue)
        Text_StanceAttackPowerValue->SetText(FText::AsNumber((int32)AtkSource.StanceRating));
}

void UStatusTabWidget::AdjustPending(EAttributeType Type, int32 Delta)
{
    APlayerController* PC = GetOwningPlayer();
    APlayerBase* Player = PC ? Cast<APlayerBase>(PC->GetPawn()) : nullptr;
    UPlayerStatComponent* StatComp = Player ? Cast<UPlayerStatComponent>(Player->GetStatComponent()) : nullptr;
    if (!StatComp) return;

    int32& Current = PendingAllocations.FindOrAdd(Type);
    const int32 NewValue = Current + Delta;

    // 남은 포인트 체크 (증가 시)
    const int32 TotalPending = [this]() {
        int32 Sum = 0;
        for (const auto& Pair : PendingAllocations) Sum += Pair.Value;
        return Sum;
        }();

    if (Delta > 0 && TotalPending >= StatComp->GetAvailableStatPoints()) return;
    if (NewValue < 0) return; // 0 밑으로는 못 내림

    Current = NewValue;
    if (Current == 0) PendingAllocations.Remove(Type);

    RefreshPreview();
}

void UStatusTabWidget::OnVitalityPlusClicked() { AdjustPending(EAttributeType::Vitality, 1); }
void UStatusTabWidget::OnVitalityMinusClicked() { AdjustPending(EAttributeType::Vitality, -1); }
void UStatusTabWidget::OnEnduranceePlusClicked() { AdjustPending(EAttributeType::Endurance, 1); }
void UStatusTabWidget::OnEnduranceMinusClicked() { AdjustPending(EAttributeType::Endurance, -1); }
void UStatusTabWidget::OnMentalityPlusClicked() { AdjustPending(EAttributeType::Mentality, 1); }
void UStatusTabWidget::OnMentalityMinusClicked() { AdjustPending(EAttributeType::Mentality, -1); }
void UStatusTabWidget::OnStrengthPlusClicked() { AdjustPending(EAttributeType::Strength, 1); }
void UStatusTabWidget::OnStrengthMinusClicked() { AdjustPending(EAttributeType::Strength, -1); }
void UStatusTabWidget::OnDexterityPlusClicked() { AdjustPending(EAttributeType::Dexterity, 1); }
void UStatusTabWidget::OnDexterityMinusClicked() { AdjustPending(EAttributeType::Dexterity, -1); }
void UStatusTabWidget::OnAffinityPlusClicked() { AdjustPending(EAttributeType::Affinity, 1); }
void UStatusTabWidget::OnAffinityMinusClicked() { AdjustPending(EAttributeType::Affinity, -1); }

void UStatusTabWidget::OnConfirmClicked()
{
    if (PendingAllocations.Num() == 0) return;

    APlayerController* PC = GetOwningPlayer();
    APlayerBase* Player = PC ? Cast<APlayerBase>(PC->GetPawn()) : nullptr;
    UPlayerStatComponent* StatComp = Player ? Cast<UPlayerStatComponent>(Player->GetStatComponent()) : nullptr;
    if (!StatComp) return;

    if (StatComp->CommitAttributeAllocation(PendingAllocations))
    {
        ResetPending();
        RefreshStats(); // 확정된 실제 값으로 갱신
    }
}

void UStatusTabWidget::ResetPending()
{
    PendingAllocations.Empty();
    RefreshPreview();
}

void UStatusTabWidget::RefreshPreview()
{
    APlayerController* PC = GetOwningPlayer();
    APlayerBase* Player = PC ? Cast<APlayerBase>(PC->GetPawn()) : nullptr;
    UPlayerStatComponent* StatComp = Player ? Cast<UPlayerStatComponent>(Player->GetStatComponent()) : nullptr;
    if (!StatComp) return;

    const FPlayerStats Preview = StatComp->PreviewStatsWithAttributeDelta(PendingAllocations);
    const FPlayerStats Current = StatComp->GetCharacterStats_Native();

    auto SetBonus = [](UTextBlock* TB, int32 Delta)
        {
            if (!TB) return;
            if (Delta > 0)
            {
                TB->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Delta)));
                TB->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                TB->SetVisibility(ESlateVisibility::Collapsed);
            }
        };

    auto GetPending = [this](EAttributeType Type) -> int32
        {
            const int32* Found = PendingAllocations.Find(Type);
            return Found ? *Found : 0;
        };

    SetBonus(Text_VitalityValue_Bonus, GetPending(EAttributeType::Vitality));
    SetBonus(Text_EnduranceValue_Bonus, GetPending(EAttributeType::Endurance));
    SetBonus(Text_MentalityValue_Bonus, GetPending(EAttributeType::Mentality));
    SetBonus(Text_StrengthValue_Bonus, GetPending(EAttributeType::Strength));
    SetBonus(Text_DexterityValue_Bonus, GetPending(EAttributeType::Dexterity));
    SetBonus(Text_AffinityValue_Bonus, GetPending(EAttributeType::Affinity));

    SetBonus(Text_HPValue_Bonus, (int32)(Preview.BaseStats.Health.Max - Current.BaseStats.Health.Max));
    SetBonus(Text_SPValue_Bonus, (int32)(Preview.Stamina.Max - Current.Stamina.Max));
    SetBonus(Text_FPValue_Bonus, (int32)(Preview.Focus.Max - Current.Focus.Max));
    SetBonus(Text_EquipLoadValue_Bonus, (int32)(Preview.EquipLoad.Max - Current.EquipLoad.Max));
    SetBonus(Text_SPRegenValue_Bonus, (int32)(Preview.CombatStats.StaminaRegenBonus - Current.CombatStats.StaminaRegenBonus));
    SetBonus(Text_PoiseValue_Bonus, (int32)(Preview.BaseStats.GetMaxPoise() - Current.BaseStats.GetMaxPoise()));
    

    if (UEquipmentComponent* EquipComp = Player->FindComponentByClass<UEquipmentComponent>())
    {
        const FAttackDamageSource CurrentAtk = EquipComp->GetAttackDamageSource();
        const FAttackDamageSource PreviewAtk = EquipComp->PreviewAttackDamageSource(
            Preview.CombatStats.StrengthAttackBonus,
            Preview.CombatStats.DexterityAttackBonus,
            0.f); // Affinity 보정 아직 미구현

        SetBonus(Text_PhysicalAttackPowerValue_Bonus, FMath::RoundToInt(PreviewAtk.AttackRating - CurrentAtk.AttackRating));
        SetBonus(Text_PoiseAttackPowerValue_Bonus, FMath::RoundToInt(PreviewAtk.PoiseRating - CurrentAtk.PoiseRating));
        SetBonus(Text_StanceAttackPowerValue_Bonus, FMath::RoundToInt(PreviewAtk.StanceRating - CurrentAtk.StanceRating));
    }

    if (Text_RemainingPointsValue)
    {
        int32 TotalPending = 0;
        for (const auto& Pair : PendingAllocations) TotalPending += Pair.Value;
        Text_RemainingPointsValue->SetText(FText::AsNumber(StatComp->GetAvailableStatPoints() - TotalPending));
    }
}