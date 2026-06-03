// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/FullScreenWidget.h"
#include "GameOverWidget.generated.h"

class UButton;
class UImage;

UCLASS(Abstract)
class UE5PROJECT_API UGameOverWidget : public UFullScreenWidget
{
	GENERATED_BODY()
	
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> BG_Overlay;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> BG_Band;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RestartButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MainMenuButton;

    // UMG Animation 자동 바인딩 (이름이 정확히 일치해야 함)
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    TObjectPtr<UWidgetAnimation> FadeInSequence;

private:
    UFUNCTION()
    void OnRestartClicked();

    UFUNCTION()
    void OnMainMenuClicked();

    UFUNCTION()
    void OnFadeInFinished();

    FWidgetAnimationDynamicEvent FadeInFinishedDelegate;

public:
    virtual void ShowWidget() override;
    virtual void HideWidget() override;
};
