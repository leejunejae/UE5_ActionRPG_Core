// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "Characters/Data/CharacterStatData.h"
#include "OverheadHPWidget.generated.h"

class UProgressBar;
class UStatComponent;
class ULockOnComponent;

UCLASS(Abstract)
class UE5PROJECT_API UOverheadHPWidget : public UBaseUserWidget
{
	GENERATED_BODY()
	
public:
    /** 표시할 적의 StatComponent를 바인딩 */
    void BindToStatComponent(UStatComponent* InStatComponent);

    /** 락온 변화 알림용 — PlayerController의 LockOnComponent에서 호출 */
    void OnLockOnChanged(bool bIsLockedOnThisEnemy);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;

    /** 데미지 후 표시 유지 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "Display")
    float DamageDisplayDuration = 3.0f;

private:
    void HandleResourceStatChanged(EResourceStatType StatType, float Current, float Max);

    void HandleDamageDisplayTimer();
    void RefreshVisibility();

    UPROPERTY()
    TObjectPtr<UStatComponent> BoundStatComponent;

    // 표시 상태
    bool bIsDamageDisplay = false;  // 데미지 받은 후 일시 표시 중인가
    bool bIsLockedOn = false;       // 락온 중인가

    FTimerHandle DamageDisplayTimerHandle;
};
