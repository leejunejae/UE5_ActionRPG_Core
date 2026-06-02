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

protected:
    /** 이 위젯이 표시되어야 할 화면 상태 */
    UPROPERTY(EditDefaultsOnly, Category = "Screen")
    EGameScreenState TargetScreenState = EGameScreenState::Startup;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void HandleScreenStateChanged(EGameScreenState NewState);
};
