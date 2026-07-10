// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Menu/Equipment/StatRequirementRowWidget.h"
#include "Components/TextBlock.h"

EFulfillmentTier UStatRequirementRowWidget::CalcFulfillmentTier(float FulfillRatio)
{
	if (FulfillRatio >= 1.0f) return EFulfillmentTier::Full;
	if (FulfillRatio >= 0.8f) return EFulfillmentTier::High;
	if (FulfillRatio >= 0.5f) return EFulfillmentTier::Mid;
	return EFulfillmentTier::Low;
}

void UStatRequirementRowWidget::InitRow(const FText& StatLabel, const FWeaponRequirementRow& Row)
{
	if (Text_CorrectionLabel) Text_CorrectionLabel->SetText(StatLabel);
	if (Text_GradeValue) Text_GradeValue->SetText(UEnum::GetDisplayValueAsText(Row.Grade));

	if (Text_RequirementLabel) Text_RequirementLabel->SetText(StatLabel);
	if (Text_ReqValue) Text_ReqValue->SetText(FText::AsNumber(Row.RequiredValue));

	if (Text_CurValue)
	{
		Text_CurValue->SetText(FText::FromString(FString::Printf(TEXT("(%d)"), Row.CurrentValue)));
	}

	OnAdoptedChanged(Row.bIsAdopted);
	OnFulfillmentTierChanged(CalcFulfillmentTier(Row.FulfillRatio));
	OnGradeChanged(Row.Grade);
}