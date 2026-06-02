// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/FullScreenWidget.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"

#include "Utils/CoreLog.h"

void UFullScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->OnScreenStateChanged.AddDynamic(this, &UFullScreenWidget::HandleScreenStateChanged);
        }
    }
}

void UFullScreenWidget::NativeDestruct()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->OnScreenStateChanged.RemoveDynamic(this, &UFullScreenWidget::HandleScreenStateChanged);
        }
    }
    Super::NativeDestruct();
}

void UFullScreenWidget::ShowWidget()
{
    // 이미 보이는 상태면 중복 알림 방지
    if (GetVisibility() != ESlateVisibility::Collapsed) return;

    Super::ShowWidget();

    // 입력 모드는 직접 안 건드림. Subsystem에 알리기만.
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->NotifyWidgetShown();
        }
    }
}

void UFullScreenWidget::HideWidget()
{
    // 이미 숨겨진 상태면 중복 알림 방지
    if (GetVisibility() == ESlateVisibility::Collapsed) return;

    Super::HideWidget();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->NotifyWidgetHidden();
        }
    }
}

void UFullScreenWidget::HandleScreenStateChanged(EGameScreenState NewState)
{
    if (NewState == TargetScreenState)
    {
        ShowWidget();
    }
    else
    {
        HideWidget();
    }
}