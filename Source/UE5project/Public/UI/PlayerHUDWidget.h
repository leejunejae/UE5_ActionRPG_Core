// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/GameHUDWidget.h"
#include "Characters/Data/CharacterStatData.h"
#include "PlayerHUDWidget.generated.h"

class UProgressBar;
class UPlayerStatComponent;

UCLASS(Abstract)
class UE5PROJECT_API UPlayerHUDWidget : public UGameHUDWidget
{
    GENERATED_BODY()

public:
    /** 어느 StatComponent를 표시할지 연결. PlayerController가 호출. */
    void BindToStatComponent(UPlayerStatComponent* InStatComponent);

protected:
    virtual void NativeDestruct() override;

    // 디자이너에서 같은 이름의 ProgressBar를 만들면 자동 바인딩
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> StaminaBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> FocusBar;

private:
    /** 리소스 스탯 변경 콜백 */
    void HandleResourceStatChanged(EResourceStatType StatType, float Current, float Max);

    UPROPERTY()
    TObjectPtr<UPlayerStatComponent> BoundStatComponent;
};
