#include "UI/StartupScreenWidget.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"

void UStartupScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    bIsFocusable = true;  // 키보드 입력 수신 활성화
}

void UStartupScreenWidget::ShowWidget()
{
    MenuState = EStartupMenuState::Appearing;
    FocusedIndex = 0;

    Super::ShowWidget();

    SetUserFocus(GetOwningPlayer());
}

// ===== BP → C++ 콜백 =====

void UStartupScreenWidget::NotifyPressAnyKeyReady()
{
    MenuState = EStartupMenuState::PressAnyKey;
}

void UStartupScreenWidget::NotifyMenuReady()
{
    MenuState = EStartupMenuState::Menu;
    OnFocusChanged(-1, FocusedIndex);
    SetUserFocus(GetOwningPlayer());  // 애니메이션 후 포커스 재확보
}

void UStartupScreenWidget::ExecuteCurrentSelection()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UUIManagerSubsystem* UIMgr = GI->GetSubsystem<UUIManagerSubsystem>();
    if (!UIMgr) return;

    switch (GetFocusedItem())
    {
    case EStartupMenuItem::NewGame:
        UIMgr->TravelToGameMap();
        break;

    case EStartupMenuItem::Exit:
        UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
        break;

    default:
        MenuState = EStartupMenuState::Menu;
        break;
    }
}

void UStartupScreenWidget::SetHoveredItem(int32 Index)
{
    if (MenuState != EStartupMenuState::Menu) return;
    if (Index == FocusedIndex) return;

    int32 OldIndex = FocusedIndex;
    FocusedIndex = Index;
    OnFocusChanged(OldIndex, FocusedIndex);
}

void UStartupScreenWidget::ConfirmSelection()
{
    if (!IsIndexEnabled(FocusedIndex)) return;

    MenuState = EStartupMenuState::Transitioning;
    OnItemSelected(GetFocusedItem());
}

// ===== 입력 처리 =====

FReply UStartupScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (MenuState == EStartupMenuState::PressAnyKey)
    {
        if (Key != EKeys::LeftShift && Key != EKeys::RightShift &&
            Key != EKeys::LeftControl && Key != EKeys::RightControl &&
            Key != EKeys::LeftAlt && Key != EKeys::RightAlt)
        {
            HandleAnyKeyPress();
            return FReply::Handled();
        }
    }
    else if (MenuState == EStartupMenuState::Menu)
    {
        if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
        {
            NavigateMenu(-1);
            return FReply::Handled();
        }
        if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
        {
            NavigateMenu(1);
            return FReply::Handled();
        }
        if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
        {
            ConfirmSelection();
            return FReply::Handled();
        }
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UStartupScreenWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // PressAnyKey 상태만 처리 — 메뉴 클릭은 Button이 담당
    if (MenuState == EStartupMenuState::PressAnyKey)
    {
        HandleAnyKeyPress();
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
// ===== 내부 로직 =====

void UStartupScreenWidget::HandleAnyKeyPress()
{
    MenuState = EStartupMenuState::Transitioning;
    OnAnyKeyPressed();
}

void UStartupScreenWidget::NavigateMenu(int32 Direction)
{
    int32 OldIndex = FocusedIndex;
    int32 NewIndex = FMath::Clamp(FocusedIndex + Direction, 0, ItemCount - 1);

    if (NewIndex != OldIndex)
    {
        FocusedIndex = NewIndex;
        OnFocusChanged(OldIndex, NewIndex);
    }
}

bool UStartupScreenWidget::IsIndexEnabled(int32 Index) const
{
    switch (static_cast<EStartupMenuItem>(Index))
    {
    case EStartupMenuItem::NewGame: return true;
    case EStartupMenuItem::Continue: return bContinueEnabled;
    case EStartupMenuItem::Settings: return bSettingsEnabled;
    case EStartupMenuItem::Exit:    return true;
    default: return false;
    }
}

bool UStartupScreenWidget::IsItemEnabled(EStartupMenuItem Item) const
{
    return IsIndexEnabled(static_cast<int32>(Item));
}