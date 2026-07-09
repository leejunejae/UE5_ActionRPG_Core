// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "Characters/Data/CharacterStatData.h"
#include "StatusTabWidget.generated.h"

/**
 * 
 */
class UTextBlock;
class UButton;
class UProgressBar;
class UPlayerStatComponent;
class APlayerBase;

UCLASS(Abstract)
class UE5PROJECT_API UStatusTabWidget : public UBaseUserWidget
{
	GENERATED_BODY()

public:
    void InitializeWithPlayer(APlayerBase* InPlayer);

    UFUNCTION(BlueprintCallable, Category = "UI")
        void RefreshStats();

protected:
    virtual void NativeConstruct() override;

    // ---- 능력치 ----
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_HPValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> ProgressBar_HP;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SPValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> ProgressBar_SP;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_FPValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> ProgressBar_FP;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_EquipLoadValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> ProgressBar_EquipLoad;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SPRegenValue;

#pragma region Attributes
protected:
    // ---- 특성 ----
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_VitalityValue;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Vitality_Plus;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Vitality_Minus;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_EnduranceValue;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Endurance_Plus;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Endurance_Minus;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_MentalityValue;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Mentality_Plus;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Mentality_Minus;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StrengthValue;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Strength_Plus;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Strength_Minus;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_DexterityValue;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Dexterity_Plus;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Dexterity_Minus;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_AffinityValue;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Affinity_Plus;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Affinity_Minus;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_ConfirmAllocation;

    // ---- 특성 배분 미리보기 텍스트 (+N 표시, 가분배 시만 Visible) ----
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_VitalityValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_EnduranceValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_MentalityValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StrengthValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_DexterityValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_AffinityValue_Bonus;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_RemainingPointsValue;

    // HP/스태미나/포커스/장비하중 특성 배분 미리보기
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_HPValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SPValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_FPValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_EquipLoadValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SPRegenValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PoiseValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PhysicalAttackPowerValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PoiseAttackPowerValue_Bonus;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StanceAttackPowerValue_Bonus;

private:
    UFUNCTION() void OnVitalityPlusClicked();
    UFUNCTION() void OnVitalityMinusClicked();
    UFUNCTION() void OnEnduranceePlusClicked();
    UFUNCTION() void OnEnduranceMinusClicked();
    UFUNCTION() void OnMentalityPlusClicked();
    UFUNCTION() void OnMentalityMinusClicked();
    UFUNCTION() void OnStrengthPlusClicked();
    UFUNCTION() void OnStrengthMinusClicked();
    UFUNCTION() void OnDexterityPlusClicked();
    UFUNCTION() void OnDexterityMinusClicked();
    UFUNCTION() void OnAffinityPlusClicked();
    UFUNCTION() void OnAffinityMinusClicked();
    UFUNCTION() void OnConfirmClicked();

    void AdjustPending(EAttributeType Type, int32 Delta);
    void RefreshPreview();
    void ResetPending();

    TMap<EAttributeType, int32> PendingAllocations;
#pragma endregion Attributes
protected:
    // ---- 공격력 ----
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PhysicalAttackPowerValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PoiseAttackPowerValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StanceAttackPowerValue;

    // ---- 방어 ----
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PhysicalDefenseValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_MagicDefenseValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_FireResistanceValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_FrostResistanceValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PoisonResistanceValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_BleedResistanceValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_PoiseValue;
};
