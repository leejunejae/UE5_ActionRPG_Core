// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/GameMenuWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"

void UGameMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetupTabButtons();

    // 생성 시 숨김 상태로 시작
    SetVisibility(ESlateVisibility::Collapsed);
}

void UGameMenuWidget::SetupTabButtons()
{
    if (TabButton_Status)
        TabButton_Status->OnClicked.AddDynamic(this, &UGameMenuWidget::OnStatusTabClicked);
    if (TabButton_Equipment)
        TabButton_Equipment->OnClicked.AddDynamic(this, &UGameMenuWidget::OnEquipmentTabClicked);
    if (TabButton_Inventory)
        TabButton_Inventory->OnClicked.AddDynamic(this, &UGameMenuWidget::OnInventoryTabClicked);
    if (TabButton_Skills)
        TabButton_Skills->OnClicked.AddDynamic(this, &UGameMenuWidget::OnSkillsTabClicked);
    if (TabButton_Map)
        TabButton_Map->OnClicked.AddDynamic(this, &UGameMenuWidget::OnMapTabClicked);
    if (TabButton_Options)
        TabButton_Options->OnClicked.AddDynamic(this, &UGameMenuWidget::OnOptionsTabClicked);
}

void UGameMenuWidget::OpenToTab(EGameMenuTab Tab)
{
    bIsOpen = true;
    SetVisibility(ESlateVisibility::Visible);
    SwitchToTab(Tab);
    ApplyOpenInputMode();
    OnMenuOpened(Tab);
}

void UGameMenuWidget::CloseMenu()
{
    // 애니메이션이 있으면 OnMenuClosed → NotifyMenuClosed 순으로 호출됨.
    // 없으면 OnMenuClosed가 구현되지 않으므로 여기서 직접 처리.
    OnMenuClosed();
    NotifyMenuClosed();
}

void UGameMenuWidget::NotifyMenuClosed()
{
    bIsOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);
    ApplyCloseInputMode();
}

void UGameMenuWidget::SwitchToTab(EGameMenuTab Tab)
{
    ActiveTab = Tab;
    if (TabContent)
    {
        TabContent->SetActiveWidgetIndex(static_cast<int32>(Tab));
    }
    OnTabChanged(Tab);
}

void UGameMenuWidget::ApplyOpenInputMode()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    FInputModeUIOnly InputMode;
    PC->SetInputMode(InputMode);
    PC->SetShowMouseCursor(true);
    PC->SetPause(true);
}

void UGameMenuWidget::ApplyCloseInputMode()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    FInputModeGameOnly InputMode;
    PC->SetInputMode(InputMode);
    PC->SetShowMouseCursor(false);
    PC->SetPause(false);
}

void UGameMenuWidget::OnStatusTabClicked() { SwitchToTab(EGameMenuTab::Status); }
void UGameMenuWidget::OnEquipmentTabClicked() { SwitchToTab(EGameMenuTab::Equipment); }
void UGameMenuWidget::OnInventoryTabClicked() { SwitchToTab(EGameMenuTab::Inventory); }
void UGameMenuWidget::OnSkillsTabClicked() { SwitchToTab(EGameMenuTab::Skills); }
void UGameMenuWidget::OnMapTabClicked() { SwitchToTab(EGameMenuTab::Map); }
void UGameMenuWidget::OnOptionsTabClicked() { SwitchToTab(EGameMenuTab::Options); }