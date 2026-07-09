// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Screen/LoadingScreenWidget.h"
#include "Utils/CoreLog.h"

void ULoadingScreenWidget::ShowWidget()
{
    Super::ShowWidget();
    if (BackgroundImages.IsEmpty() && LoadingTips.IsEmpty()) return;

    CurrentIndex = BackgroundImages.Num() > 0
        ? FMath::RandRange(0, BackgroundImages.Num() - 1) : 0;

    DoCycleContent(true);  // 첫 표시: FadeIn만

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            CycleTimerHandle,
            this, &ULoadingScreenWidget::DoCycleContentTimer,
            CycleInterval, true
        );
    }
}

void ULoadingScreenWidget::HideWidget()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CycleTimerHandle);
    }
    Super::HideWidget();
}

void ULoadingScreenWidget::DoCycleContentTimer()
{
    DoCycleContent(false);  // 이후 사이클: FadeOut→swap→FadeIn
}

void ULoadingScreenWidget::DoCycleContent(bool bIsFirstShow)
{
    UTexture2D* Bg = BackgroundImages.Num() > 0
        ? BackgroundImages[CurrentIndex % BackgroundImages.Num()] : nullptr;
    const FLoadingTipData Tip = LoadingTips.Num() > 0 ? LoadingTips[CurrentIndex % LoadingTips.Num()] : FLoadingTipData();

    OnCycleContent(Bg, Tip.TipTitle, Tip.TipContent, bIsFirstShow);
    CurrentIndex++;
}