// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/OverheadHPWidget.h"
#include "Components/ProgressBar.h"
#include "Characters/Components/StatComponent.h"
#include "TimerManager.h"

void UOverheadHPWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 처음엔 숨김 상태로 시작
    SetVisibility(ESlateVisibility::Hidden);
}

void UOverheadHPWidget::NativeDestruct()
{
    if (BoundStatComponent)
    {
        BoundStatComponent->OnResourceStatChanged.RemoveAll(this);
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DamageDisplayTimerHandle);
    }
    Super::NativeDestruct();
}

void UOverheadHPWidget::BindToStatComponent(UStatComponent* InStatComponent)
{
    if (!InStatComponent) return;

    if (BoundStatComponent)
    {
        BoundStatComponent->OnResourceStatChanged.RemoveAll(this);
    }

    BoundStatComponent = InStatComponent;

    BoundStatComponent->OnResourceStatChanged.AddUObject(
        this, &UOverheadHPWidget::HandleResourceStatChanged);

    // 초기값 표시 (보이지는 않지만 값은 맞춰둠)
    const FCharacterStats& Stats = BoundStatComponent->GetCommonStats();
    if (HealthBar)
    {
        HealthBar->SetPercent(Stats.GetHealthPercent());
    }
}

void UOverheadHPWidget::HandleResourceStatChanged(EResourceStatType StatType, float Current, float Max)
{
    if (StatType != EResourceStatType::Health) return;

    const float Percent = (Max > KINDA_SMALL_NUMBER) ? (Current / Max) : 0.0f;
    if (HealthBar)
    {
        HealthBar->SetPercent(Percent);
    }

    // 데미지/회복 모두 일시 표시 트리거 (값 변화가 곧 표시 신호)
    bIsDamageDisplay = true;

    // 타이머 리셋: 이미 돌고 있던 타이머가 있으면 다시 시작
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            DamageDisplayTimerHandle,
            this, &UOverheadHPWidget::HandleDamageDisplayTimer,
            DamageDisplayDuration, false);
    }

    RefreshVisibility();
}

void UOverheadHPWidget::HandleDamageDisplayTimer()
{
    bIsDamageDisplay = false;
    RefreshVisibility();
}

void UOverheadHPWidget::OnLockOnChanged(bool bIsLockedOnThisEnemy)
{
    bIsLockedOn = bIsLockedOnThisEnemy;
    RefreshVisibility();
}

void UOverheadHPWidget::RefreshVisibility()
{
    // 정책 C: 둘 중 하나라도 활성이면 표시
    const bool bShouldShow = bIsDamageDisplay || bIsLockedOn;
    SetVisibility(bShouldShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}