// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "Core/Subsystems/GameInstanceSystem/UIManagerSubsystem.h"
#include "FullScreenWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class UE5PROJECT_API UFullScreenWidget : public UBaseUserWidget
{
    GENERATED_BODY()

public:
    virtual void ShowWidget() override;
    virtual void HideWidget() override;

    UFUNCTION(BlueprintCallable, Category = "Widget")
    void FinishHideWidget();

protected:
    /** 표시될 때 페이드인 재생 (BP에서 구현) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Widget")
    void OnShowAnimation();

    /** 숨겨질 때 페이드아웃 재생 (BP에서 구현)
 *  끝나면 반드시 FinishHideWidget() 호출할 것
 *  구현 안 하면 즉시 숨김 (기존 동작 유지) */
    UFUNCTION(BlueprintNativeEvent, Category = "Widget")
    void OnHideAnimation();
    virtual void OnHideAnimation_Implementation();

    /** 이 위젯이 표시되어야 할 화면 상태 */
    UPROPERTY(EditDefaultsOnly, Category = "Screen")
    EGameScreenState TargetScreenState = EGameScreenState::Startup;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void HandleScreenStateChanged(EGameScreenState NewState);
};
