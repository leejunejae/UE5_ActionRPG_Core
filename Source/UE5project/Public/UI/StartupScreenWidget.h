#pragma once

#include "CoreMinimal.h"
#include "UI/FullScreenWidget.h"
#include "StartupScreenWidget.generated.h"

UENUM(BlueprintType)
enum class EStartupMenuState : uint8
{
    Appearing       UMETA(DisplayName = "Appearing"),
    PressAnyKey     UMETA(DisplayName = "PressAnyKey"),
    Transitioning   UMETA(DisplayName = "Transitioning"),
    Menu            UMETA(DisplayName = "Menu"),
};

UENUM(BlueprintType)
enum class EStartupMenuItem : uint8
{
    NewGame     UMETA(DisplayName = "New Game"),
    Continue    UMETA(DisplayName = "Continue"),
    Settings    UMETA(DisplayName = "Settings"),
    Exit        UMETA(DisplayName = "Exit"),
};

UCLASS(Abstract)
class UE5PROJECT_API UStartupScreenWidget : public UFullScreenWidget
{
    GENERATED_BODY()

public:
    virtual void ShowWidget() override;

    // ===== BP → C++ 콜백 =====
    UFUNCTION(BlueprintCallable, Category = "Startup")
    void NotifyPressAnyKeyReady();

    UFUNCTION(BlueprintCallable, Category = "Startup")
    void NotifyMenuReady();

    UFUNCTION(BlueprintCallable, Category = "Startup")
    void ExecuteCurrentSelection();

    /** 마우스 호버 시 BP 아이템 위젯의 OnMouseEnter에서 호출 */
    UFUNCTION(BlueprintCallable, Category = "Startup")
    void SetHoveredItem(int32 Index);

    /** 선택 확정 — 키보드/마우스 공용. BP에서 직접 호출 가능 */
    UFUNCTION(BlueprintCallable, Category = "Startup")
    void ConfirmSelection();

    // ===== BP 조회 =====
    UFUNCTION(BlueprintPure, Category = "Startup")
    EStartupMenuItem GetFocusedItem() const { return static_cast<EStartupMenuItem>(FocusedIndex); }

    UFUNCTION(BlueprintPure, Category = "Startup")
    bool IsItemEnabled(EStartupMenuItem Item) const;

    UFUNCTION(BlueprintPure, Category = "Startup")
    EStartupMenuState GetMenuState() const { return MenuState; }

protected:
    virtual void NativeConstruct() override;

    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // ===== C++ → BP 애니메이션 이벤트 =====
    UFUNCTION(BlueprintImplementableEvent, Category = "Startup|Animation")
    void OnAnyKeyPressed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Startup|Navigation")
    void OnFocusChanged(int32 OldIndex, int32 NewIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Startup|Animation")
    void OnItemSelected(EStartupMenuItem Item);

    // ===== 설정 =====
    UPROPERTY(EditDefaultsOnly, Category = "Startup|Config")
    bool bContinueEnabled = false;

    UPROPERTY(EditDefaultsOnly, Category = "Startup|Config")
    bool bSettingsEnabled = false;

private:
    EStartupMenuState MenuState = EStartupMenuState::Appearing;
    int32 FocusedIndex = 0;

    static constexpr int32 ItemCount = 4;

    void HandleAnyKeyPress();
    void NavigateMenu(int32 Direction);
    bool IsIndexEnabled(int32 Index) const;
};
