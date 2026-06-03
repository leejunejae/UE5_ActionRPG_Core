// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/StartupScreenWidget.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

void UStartupScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (StartButton)
    {
        StartButton->OnClicked.AddDynamic(this, &UStartupScreenWidget::OnStartClicked);
    }
    if (OptionsButton)
    {
        OptionsButton->OnClicked.AddDynamic(this, &UStartupScreenWidget::OnOptionsClicked);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UStartupScreenWidget::OnQuitClicked);
    }
}

void UStartupScreenWidget::OnStartClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>())
        {
            UIMgr->SetScreenState(EGameScreenState::Loading);
        }
    }
}

void UStartupScreenWidget::OnOptionsClicked()
{
    // Step E(옵션 메뉴)에서 구현 예정
}

void UStartupScreenWidget::OnQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}