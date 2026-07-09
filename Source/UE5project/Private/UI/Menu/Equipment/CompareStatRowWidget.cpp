// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Menu/Equipment/CompareStatRowWidget.h"

#include "Components/TextBlock.h"

void UCompareStatRowWidget::InitRow(const FText& StatLabel, float CurrentValue, float NewValue)
{
	if (Text_Label) Text_Label->SetText(StatLabel);
	if (Text_Current) Text_Current->SetText(FText::AsNumber((int32)CurrentValue));
	if (Text_New) Text_New->SetText(FText::AsNumber((int32)NewValue));

	const int32 Delta = (int32)(NewValue - CurrentValue);

	if (Text_Delta)
	{
		Text_Delta->SetText(FText::FromString(FString::Printf(TEXT("%s%d"), Delta > 0 ? TEXT("+") : TEXT(""), Delta)));

		if (Delta > 0)
		{
			Text_Delta->SetColorAndOpacity(FSlateColor(PositiveColor));
		}
		else if (Delta < 0)
		{
			Text_Delta->SetColorAndOpacity(FSlateColor(NegativeColor));
		}
		else
		{
			Text_Delta->SetColorAndOpacity(FSlateColor(NeutralColor));
		}
	}
}