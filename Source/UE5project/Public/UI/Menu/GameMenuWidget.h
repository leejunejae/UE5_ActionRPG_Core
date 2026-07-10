// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "GameMenuWidget.generated.h"

/**
 * 
 */
class UWidgetSwitcher;
class UButton;

UCLASS(Abstract)
class UE5PROJECT_API UGameMenuWidget : public UBaseUserWidget
{
    GENERATED_BODY()

public:
    /**
     * 특정 탭으로 메뉴 열기.
     * 이미 열려있고 같은 탭이면 닫힘 (토글).
     * 이미 열려있고 다른 탭이면 탭만 전환.
     */
    void OpenToTab(EGameMenuTab Tab);

    /** 메뉴 닫기 */
    void CloseMenu();

    bool IsOpen() const { return bIsOpen; }
    EGameMenuTab GetActiveTab() const { return ActiveTab; }

protected:
    virtual void NativeConstruct() override;

    // ---- BindWidget (WBP에서 이름 일치 필수) ----

    UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
    TObjectPtr<UWidgetSwitcher> TabContent;

    UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
    TObjectPtr<UButton> TabButton_Status;

    UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
    TObjectPtr<UButton> TabButton_Equipment;

    UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
    TObjectPtr<UButton> TabButton_Inventory;

    UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
    TObjectPtr<UButton> TabButton_Skills;

    UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
    TObjectPtr<UButton> TabButton_Map;

    // ---- BP 이벤트 ----

    /** 탭 전환 시 — 탭 버튼 하이라이트 등 BP에서 처리 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
    void OnTabChanged(EGameMenuTab NewTab);

    /** 메뉴 열릴 때 — 페이드인 등 BP에서 처리 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
    void OnMenuOpened(EGameMenuTab InitialTab);

    /**
     * 메뉴 닫힐 때 — 페이드아웃 등 BP에서 처리.
     * 애니메이션이 있으면 끝에 NotifyMenuClosed() 호출.
     * 없으면 구현 안 해도 됨 (C++에서 즉시 처리).
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
    void OnMenuClosed();

    /** BP의 닫힘 애니메이션 끝에 호출 — Collapsed 처리 */
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void NotifyMenuClosed();

private:
    void SwitchToTab(EGameMenuTab Tab);
    void SetupTabButtons();
    void ApplyOpenInputMode();
    void ApplyCloseInputMode();

    UFUNCTION() void OnStatusTabClicked();
    UFUNCTION() void OnEquipmentTabClicked();
    UFUNCTION() void OnInventoryTabClicked();
    UFUNCTION() void OnSkillsTabClicked();
    UFUNCTION() void OnMapTabClicked();

    EGameMenuTab ActiveTab = EGameMenuTab::Status;
    bool bIsOpen = false;
};