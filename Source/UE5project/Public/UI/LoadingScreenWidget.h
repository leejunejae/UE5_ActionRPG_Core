// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/FullScreenWidget.h"
#include "LoadingScreenWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class UE5PROJECT_API ULoadingScreenWidget : public UFullScreenWidget
{
	GENERATED_BODY()
	
protected:
    /** 배경 이미지 목록 — BP에서 배열에 텍스처 추가 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading|Content")
    TArray<TObjectPtr<UTexture2D>> BackgroundImages;

    /** 팁/분위기 문구 목록 — BP에서 FText 배열로 추가 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading|Content")
    TArray<FText> LoadingTips;

    /** 콘텐츠 전환 간격 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading|Content")
    float CycleInterval = 5.0f;

    virtual void ShowWidget() override;
    virtual void HideWidget() override;

    /**
     * 타이머가 주기적으로 호출 — BP에서 페이드 애니메이션 + 이미지/텍스트 교체 구현
     * Background: 다음에 표시할 배경 (배열 비어있으면 nullptr)
     * Tip: 다음에 표시할 문구 (배열 비어있으면 FText::GetEmpty())
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Loading")
    void OnCycleContent(UTexture2D* Background, const FText& Tip, bool bIsFirstShow);

private:
    void DoCycleContent(bool bIsFirstShow);
    void DoCycleContentTimer();  // 타이머용 래퍼 (bool 없이 호출)

    FTimerHandle CycleTimerHandle;
    int32 CurrentIndex = 0;
};
