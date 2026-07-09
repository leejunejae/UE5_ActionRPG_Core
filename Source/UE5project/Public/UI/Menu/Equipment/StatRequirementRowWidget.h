// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "StatRequirementRowWidget.generated.h"


class UTextBlock;
class UProgressBar;

UCLASS(Abstract)
class UE5PROJECT_API UStatRequirementRowWidget : public UBaseUserWidget
{
	GENERATED_BODY()
	
public:
	void InitRow(const FText& StatLabel, const FWeaponRequirementRow& Row);

protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StatLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ReqValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_CurValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_GradeValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_AppliedValue;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> ProgressBar_Fulfill;

	// bIsAdopted일 때 배지/강조 표시는 BP에서 처리 (탭바 하이라이트와 같은 패턴)
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnAdoptedChanged(bool bIsAdopted);
};
