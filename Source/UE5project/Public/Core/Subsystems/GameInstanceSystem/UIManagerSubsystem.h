// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UIManagerSubsystem.generated.h"

class UFullScreenWidget;
class UUIConfigDataAsset;
class UWorld;

UENUM(BlueprintType)
enum class EGameScreenState : uint8
{
    Startup     UMETA(DisplayName = "Startup"),
    Loading     UMETA(DisplayName = "Loading"),
    InGame      UMETA(DisplayName = "InGame"),
    GameOver    UMETA(DisplayName = "GameOver")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScreenStateChanged, EGameScreenState, NewState);

UCLASS()
class UE5PROJECT_API UUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    // USubsystem 인터페이스
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ===== 화면 상태 관리 =====
    UPROPERTY(BlueprintAssignable, Category = "UI|Screen")
    FOnScreenStateChanged OnScreenStateChanged;

    UFUNCTION(BlueprintCallable, Category = "UI|Screen")
    void SetScreenState(EGameScreenState NewState);

    UFUNCTION(BlueprintPure, Category = "UI|Screen")
    EGameScreenState GetScreenState() const { return CurrentState; }

    // ===== 전체 화면 위젯 관리 =====
    /** PlayerController가 준비된 후 호출하여 전체 화면 위젯들을 생성 */
    UFUNCTION(BlueprintCallable, Category = "UI|Widget")
    void CreateFullScreenWidgets(APlayerController* OwningPlayer);

    // ===== 입력 모드 중앙 관리 =====
    /** 전체화면 위젯이 표시 */
    void NotifyWidgetShown();

    /** 전체화면 위젯이 숨김 */
    void NotifyWidgetHidden();

private:
    /** 표시 중인 전체화면 위젯 개수에 따라 입력 모드 재계산 */
    void RefreshInputMode();

    UPROPERTY()
    EGameScreenState CurrentState = EGameScreenState::Startup;

    UPROPERTY()
    TObjectPtr<UUIConfigDataAsset> UIConfig;

    UPROPERTY()
    TArray<TObjectPtr<UFullScreenWidget>> CreatedWidgets;

    /** 현재 화면에 떠 있는 전체화면 위젯 수 */
    int32 VisibleFullScreenWidgetCount = 0;

    /** 로딩 화면 최소 표시 시간(초) — 빠른 로드 시 깜빡임 방지 */
    float MinLoadingScreenTime = 5.0f;

    float LoadingScreenStartTime = 0.0f;
    FTimerHandle MinTimeTimerHandle;

    bool bWidgetsCreated = false;

#pragma region Level
public:
    /** "게임 시작" 버튼 등에서 호출 — GameMap으로 이동(로딩 화면 + InGame 진입) */
    UFUNCTION(BlueprintCallable, Category = "UI|Flow")
    void TravelToGameMap();

    /** 게임오버 "메인 메뉴로" 등에서 호출 — StartupMap으로 이동 */
    UFUNCTION(BlueprintCallable, Category = "UI|Flow")
    void TravelToStartupMap();

private:
    /** 로딩 화면 등록 후 해당 레벨로 OpenLevel */
    void TravelToLevel(const TSoftObjectPtr<UWorld>& Level, EGameScreenState ArrivalState);

    /** 레벨 로드 직전 호출 — 이전 월드의 전체화면 위젯 정리 + 도착 상태 세팅 */
    void HandlePreLoadMap(const FString& MapName);

    void HandleMapPreloadComplete(EAsyncLoadingResult::Type Result);

    void DoTravelToNewGameMap();
    void ExecuteMapTravel();

    /** 화면 전환 페이드아웃 대기 시간 — BP 애니메이션 길이와 맞출 것 */
    float ScreenTransitionDuration = 0.5f;
    FTimerHandle TravelAfterFadeHandle;
    /** 레벨 도착 시 적용할 화면 상태 (PreLoadMap에서 CurrentState로 반영) */
    EGameScreenState PendingArrivalState = EGameScreenState::Startup;

    FDelegateHandle PreLoadMapHandle;
#pragma endregion Level
};
