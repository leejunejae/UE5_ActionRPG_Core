// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UIManagerSubsystem.generated.h"

/**
 *
 */
class UFullScreenWidget;
class UUIConfigDataAsset;

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
    /** 전체화면 위젯이 표시될 때 호출 */
    void NotifyWidgetShown();

    /** 전체화면 위젯이 숨겨질 때 호출 */
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

    bool bWidgetsCreated = false;
};
