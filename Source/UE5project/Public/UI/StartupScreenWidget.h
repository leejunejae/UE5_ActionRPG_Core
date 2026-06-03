// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/FullScreenWidget.h"
#include "StartupScreenWidget.generated.h"

class UButton;

UCLASS(Abstract)
class UE5PROJECT_API UStartupScreenWidget : public UFullScreenWidget
{
	GENERATED_BODY()
	
protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> StartButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> OptionsButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

private:
    UFUNCTION()
    void OnStartClicked();

    UFUNCTION()
    void OnOptionsClicked();

    UFUNCTION()
    void OnQuitClicked();
};
