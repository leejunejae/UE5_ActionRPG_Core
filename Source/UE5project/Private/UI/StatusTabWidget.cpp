// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/StatusTabWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Characters/Player/PlayerBase.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Characters/Components/EquipmentComponent.h"

void UStatusTabWidget::NativeConstruct()
{
    Super::NativeConstruct();
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