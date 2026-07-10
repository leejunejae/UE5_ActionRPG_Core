// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "StatRequirementRowWidget.generated.h"


class UTextBlock;

UENUM(BlueprintType)
enum class EFulfillmentTier : uint8
{
	Full,       // 100% 이상
	High,       // 80~99%
	Mid,        // 50~79%
	Low         // 50% 미만
};

UCLASS(Abstract)
class UE5PROJECT_API UStatRequirementRowWidget : public UBaseUserWidget
{
	GENERATED_BODY()

public:
	void InitRow(const FText& StatLabel, const FWeaponRequirementRow& Row);

protected:
	// ---- 왼쪽: 능력 보정 (등급) ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_CorrectionLabel;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Text_GradeValue;

	// ---- 오른쪽: 필요 능력치 (요구치 + 보유치) ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_RequirementLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ReqValue;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Text_CurValue;

	// bIsAdopted(3개 스탯 중 실제 채택된 스탯)일 때 강조는 BP에서 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnAdoptedChanged(bool bIsAdopted);

	// 요구치 미충족 색상 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnFulfillmentTierChanged(EFulfillmentTier Tier);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnGradeChanged(EWeaponGrade Grade);

private:
	static EFulfillmentTier CalcFulfillmentTier(float FulfillRatio);
};
