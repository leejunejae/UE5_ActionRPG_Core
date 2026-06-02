// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseUserWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class UE5PROJECT_API UBaseUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    /** 위젯을 뷰포트에 추가하고 표시 */
    UFUNCTION(BlueprintCallable, Category = "Widget")
    virtual void ShowWidget();

    /** 위젯을 뷰포트에서 제거 */
    UFUNCTION(BlueprintCallable, Category = "Widget")
    virtual void HideWidget();

protected:
    virtual void NativeConstruct() override;

    /** 블루프린트에서 표시 시점에 추가 동작 정의 가능 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Widget")
    void OnShow();

    /** 블루프린트에서 숨김 시점에 추가 동작 정의 가능 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Widget")
    void OnHide();
};
