// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/BaseUserWidget.h"

void UBaseUserWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UBaseUserWidget::ShowWidget()
{
    if (!IsInViewport())
    {
        AddToViewport();
    }
    SetVisibility(ESlateVisibility::Visible);
    OnShow();
}

void UBaseUserWidget::HideWidget()
{
    SetVisibility(ESlateVisibility::Collapsed);
    OnHide();
    // 완전히 제거하고 싶으면: RemoveFromParent();
}