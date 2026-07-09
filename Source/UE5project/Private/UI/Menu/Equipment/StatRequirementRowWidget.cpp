// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Menu/Equipment/StatRequirementRowWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UStatRequirementRowWidget::InitRow(const FText& StatLabel, const FWeaponRequirementRow& Row)
{
	if (Text_StatLabel) Text_StatLabel->SetText(StatLabel);
	if (Text_ReqValue) Text_ReqValue->SetText(FText::AsNumber(Row.RequiredValue));
	if (Text_CurValue) Text_CurValue->SetText(FText::AsNumber(Row.CurrentValue));
	if (Text_GradeValue) Text_GradeValue->SetText(UEnum::GetDisplayValueAsText(Row.Grade));
	if (Text_AppliedValue) Text_AppliedValue->SetText(FText::AsNumber((int32)Row.AppliedAttackValue));
	if (ProgressBar_Fulfill) ProgressBar_Fulfill->SetPercent(Row.FulfillRatio);

	OnAdoptedChanged(Row.bIsAdopted);
}