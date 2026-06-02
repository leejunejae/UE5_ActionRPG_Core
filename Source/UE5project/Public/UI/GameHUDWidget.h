// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "GameHUDWidget.generated.h"

UCLASS(Abstract)
class UE5PROJECT_API UGameHUDWidget : public UBaseUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void HandleScreenStateChanged(EGameScreenState NewState);
};