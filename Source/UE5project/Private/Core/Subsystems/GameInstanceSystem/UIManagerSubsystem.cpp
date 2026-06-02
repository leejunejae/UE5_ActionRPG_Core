// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "UI/FullScreenWidget.h"
#include "UI/UIConfigDataAsset.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "Utils/CoreLog.h"

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentState = EGameScreenState::Startup;

    // 데이터 에셋 로드
    static const TCHAR* ConfigPath = TEXT("/Game/08_UI/Data/FullScreenUIConfig_DA.FullScreenUIConfig_DA");
    UIConfig = LoadObject<UUIConfigDataAsset>(nullptr, ConfigPath);

    if (!UIConfig)
    {
        UE_LOG(Log_UI, Warning, TEXT("[UIManagerSubsystem] Failed to load UIConfig at %s"), ConfigPath);
    }
}

void UUIManagerSubsystem::Deinitialize()
{
    CreatedWidgets.Empty();
    Super::Deinitialize();
}

void UUIManagerSubsystem::SetScreenState(EGameScreenState NewState)
{
    if (CurrentState == NewState) return;

    CurrentState = NewState;
    OnScreenStateChanged.Broadcast(NewState);
}

void UUIManagerSubsystem::CreateFullScreenWidgets(APlayerController* OwningPlayer)
{
    if (bWidgetsCreated) return;
    if (!OwningPlayer) return;
    if (!UIConfig)
    {
        UE_LOG(Log_UI, Warning, TEXT("[UIManagerSubsystem] UIConfig is null, cannot create widgets"));
        return;
    }

    for (TSubclassOf<UFullScreenWidget> WidgetClass : UIConfig->FullScreenWidgetClasses)
    {
        if (!WidgetClass) continue;

        UFullScreenWidget* NewWidget = CreateWidget<UFullScreenWidget>(OwningPlayer, WidgetClass);
        if (NewWidget)
        {
            NewWidget->AddToViewport();
            NewWidget->SetVisibility(ESlateVisibility::Collapsed);
            CreatedWidgets.Add(NewWidget);
        }
    }

    bWidgetsCreated = true;

    // 현재 상태로 브로드캐스트해서 자기 차례인 위젯이 자동 표시되게
    OnScreenStateChanged.Broadcast(CurrentState);
}

void UUIManagerSubsystem::NotifyWidgetShown()
{
    VisibleFullScreenWidgetCount++;
    RefreshInputMode();
}

void UUIManagerSubsystem::NotifyWidgetHidden()
{
    VisibleFullScreenWidgetCount = FMath::Max(0, VisibleFullScreenWidgetCount - 1);
    RefreshInputMode();
}

void UUIManagerSubsystem::RefreshInputMode()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    if (VisibleFullScreenWidgetCount > 0)
    {
        // 전체화면 UI가 떠 있음 → UI 모드 + 커서 표시
        FInputModeUIOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }
    else
    {
        // 다 숨겨짐 → 게임 모드 + 커서 숨김
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(false);
    }
}