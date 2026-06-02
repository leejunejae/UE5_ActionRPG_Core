// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GameOverWidget.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "Components/Button.h"

void UGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (RestartButton)
    {
        RestartButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnRestartClicked);
    }
    if (MainMenuButton)
    {
        MainMenuButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnMainMenuClicked);
    }
}

void UGameOverWidget::OnRestartClicked()
{
    // Step 5에서 진짜 부활 로직 추가
    // 일단은 단순히 InGame 상태로 복귀
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->SetScreenState(EGameScreenState::InGame);
        }
    }
}

void UGameOverWidget::OnMainMenuClicked()
{
    // Step 5에서 OpenLevel(StartupMap) 추가
    // 일단은 Startup 상태로
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->SetScreenState(EGameScreenState::Startup);
        }
    }
}