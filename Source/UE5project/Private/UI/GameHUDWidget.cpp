// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GameHUDWidget.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"

void UGameHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // HUD는 클릭/입력 방해 안 되게
    SetVisibility(ESlateVisibility::HitTestInvisible);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->OnScreenStateChanged.AddDynamic(this, &UGameHUDWidget::HandleScreenStateChanged);

            if (UIMgr->GetScreenState() != EGameScreenState::InGame)
            {
                SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

void UGameHUDWidget::NativeDestruct()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->OnScreenStateChanged.RemoveDynamic(this, &UGameHUDWidget::HandleScreenStateChanged);
        }
    }
    Super::NativeDestruct();
}

void UGameHUDWidget::HandleScreenStateChanged(EGameScreenState NewState)
{
    // InGame일 때만 표시
    if (NewState == EGameScreenState::InGame)
    {
        SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}