// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Characters/Player/Components/PlayerStatComponent.h"

void UPlayerHUDWidget::BindToStatComponent(UPlayerStatComponent* InStatComponent)
{
    if (!InStatComponent) return;

    // 이전 구독이 있으면 해제
    if (BoundStatComponent)
    {
        BoundStatComponent->OnResourceStatChanged.RemoveAll(this);
    }

    BoundStatComponent = InStatComponent;

    // 1) 앞으로의 변경을 구독
    BoundStatComponent->OnResourceStatChanged.AddUObject(
        this, &UPlayerHUDWidget::HandleResourceStatChanged);

    // 2) 현재값을 즉시 한 번 읽어와서 초기 표시 (브로드캐스트를 놓쳤어도 안전)
    const FPlayerStats Stats = BoundStatComponent->GetCharacterStats_Native();

    if (HealthBar)
    {
        HealthBar->SetPercent(Stats.BaseStats.GetHealthPercent());
    }

    if (StaminaBar && Stats.Stamina.Max > KINDA_SMALL_NUMBER)
    {
        StaminaBar->SetPercent(Stats.Stamina.Current / Stats.Stamina.Max);
    }
    if (FocusBar && Stats.Focus.Max > KINDA_SMALL_NUMBER)
    {
        FocusBar->SetPercent(Stats.Focus.Current / Stats.Focus.Max);
    }
}

void UPlayerHUDWidget::NativeDestruct()
{
    if (BoundStatComponent)
    {
        BoundStatComponent->OnResourceStatChanged.RemoveAll(this);
        BoundStatComponent = nullptr;
    }
    Super::NativeDestruct();
}

void UPlayerHUDWidget::HandleResourceStatChanged(EResourceStatType StatType, float Current, float Max)
{
    const float Percent = (Max > KINDA_SMALL_NUMBER) ? (Current / Max) : 0.0f;

    switch (StatType)
    {
    case EResourceStatType::Health:
        if (HealthBar) HealthBar->SetPercent(Percent);
        break;
    case EResourceStatType::Stamina:
        if (StaminaBar) StaminaBar->SetPercent(Percent);
        break;
    case EResourceStatType::Focus:
        if (FocusBar) FocusBar->SetPercent(Percent);
        break;
    }
}