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

    // ---- 특성 ----
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_VitalityValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_EnduranceValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_MentalityValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StrengthValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_DexterityValue;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_AffinityValue;

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
