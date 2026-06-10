// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "UI/FullScreenWidget.h"
#include "UI/UIConfigDataAsset.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SThrobber.h"
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

    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UUIManagerSubsystem::HandlePreLoadMap);
}

void UUIManagerSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
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

    if (CurrentState == EGameScreenState::Startup && !UIConfig->StartupMap.IsNull())
    {
        // PIE에서는 맵 이름에 "UEDPIE_0_" 같은 접두어가 붙음 — 제거 후 비교
        FString CurrentMapName = GetWorld()->GetMapName();
        CurrentMapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

        const FString StartupMapShortName = UIConfig->StartupMap.GetAssetName();

        if (!CurrentMapName.Equals(StartupMapShortName, ESearchCase::IgnoreCase))
        {
            CurrentState = EGameScreenState::InGame;
        }
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
    RefreshInputMode();
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

// ==== = 레벨 전환 ==== =

void UUIManagerSubsystem::TravelToGameMap()
{
    if (!UIConfig || UIConfig->DefaultGameMap.IsNull())
    {
        UE_LOG(Log_UI, Warning, TEXT("[UIManagerSubsystem] TravelToGameMap: NewGameMap is not set"));
        return;
    }

    PendingArrivalState = EGameScreenState::InGame;

    // 로딩 화면 먼저 표시 (StartupMap 월드가 살아있으므로 UMG 정상 작동)
    SetScreenState(EGameScreenState::Loading);

    if (UWorld* World = GetGameInstance()->GetWorld())
    {
        LoadingScreenStartTime = World->GetTimeSeconds();
    }

    // 맵 패키지를 비동기로 선로딩 — 완료되면 OpenLevel
    const FString PackageName = UIConfig->DefaultGameMap.ToSoftObjectPath().GetLongPackageName();
    FLoadPackageAsyncDelegate Callback;
    Callback.BindWeakLambda(this, [this](const FName&, UPackage*, EAsyncLoadingResult::Type Result)
        {
            HandleMapPreloadComplete(Result);
        });

    LoadPackageAsync(PackageName, Callback);
}

void UUIManagerSubsystem::TravelToStartupMap()
{
    if (!UIConfig) return;
    TravelToLevel(UIConfig->StartupMap, EGameScreenState::Startup);
}

void UUIManagerSubsystem::TravelToLevel(const TSoftObjectPtr<UWorld>& Level, EGameScreenState ArrivalState)
{
    if (Level.IsNull())
    {
        UE_LOG(Log_UI, Warning, TEXT("[UIManagerSubsystem] TravelToLevel: target level is not set in UIConfig"));
        return;
    }

    PendingArrivalState = ArrivalState;


    UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level);
}

void UUIManagerSubsystem::HandlePreLoadMap(const FString& MapName)
{
    // 이전 월드에서 만든 전체화면 위젯은 OwningPlayer가 곧 파괴돼 좀비가 됨.
    // viewport에서 떼어내고 참조를 비워 새 맵에서 재생성되게 한다.
    for (UFullScreenWidget* Widget : CreatedWidgets)
    {
        if (Widget)
        {
            Widget->RemoveFromParent();
        }
    }
    CreatedWidgets.Empty();
    bWidgetsCreated = false;
    VisibleFullScreenWidgetCount = 0;

    // 도착할 레벨에 맞는 상태로 갱신.
    // 위젯이 아직 없으니 여기선 브로드캐스트 안 함 —
    // 새 맵의 ControllerBase::BeginPlay → CreateFullScreenWidgets가 이 상태로 브로드캐스트하며 표시.
    CurrentState = PendingArrivalState;
}

void UUIManagerSubsystem::HandleMapPreloadComplete(EAsyncLoadingResult::Type Result)
{
    if (Result != EAsyncLoadingResult::Succeeded)
    {
        UE_LOG(Log_UI, Error, TEXT("[UIManagerSubsystem] Map preload failed — returning to Startup"));
        SetScreenState(EGameScreenState::Startup);
        return;
    }

    UWorld* World = GetGameInstance()->GetWorld();
    if (!World) { DoTravelToNewGameMap(); return; }

    // 최소 표시 시간이 남아있으면 그만큼 기다렸다가 전환
    float Elapsed = World->GetTimeSeconds() - LoadingScreenStartTime;
    float Remaining = MinLoadingScreenTime - Elapsed;

    if (Remaining > 0.0f)
    {
        FTimerDelegate Delegate;
        Delegate.BindUObject(this, &UUIManagerSubsystem::DoTravelToNewGameMap);
        World->GetTimerManager().SetTimer(MinTimeTimerHandle, Delegate, Remaining, false);
    }
    else
    {
        DoTravelToNewGameMap();
    }
}

void UUIManagerSubsystem::DoTravelToNewGameMap()
{
    for (UFullScreenWidget* Widget : CreatedWidgets)
    {
        if (Widget && Widget->GetVisibility() != ESlateVisibility::Collapsed)
        {
            Widget->HideWidget();  // OnHideAnimation 트리거
        }
    }

    // 페이드아웃 완료 후 OpenLevel
    UWorld* World = GetGameInstance()->GetWorld();
    if (!World) { ExecuteMapTravel(); return; }

    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &UUIManagerSubsystem::ExecuteMapTravel);
    World->GetTimerManager().SetTimer(TravelAfterFadeHandle, Delegate, ScreenTransitionDuration, false);
}

void UUIManagerSubsystem::ExecuteMapTravel()
{
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, UIConfig->DefaultGameMap);
}