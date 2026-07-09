// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "CompareStatRowWidget.generated.h"


class UTextBlock;

UCLASS(Abstract)
class UE5PROJECT_API UCompareStatRowWidget : public UBaseUserWidget
{
	GENERATED_BODY()
	
public:
	void InitRow(const FText& StatLabel, float CurrentValue, float NewValue);

protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_Label;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_Current;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_New;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_Delta;

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor PositiveColor = FLinearColor(0.4f, 0.8f, 0.4f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor NegativeColor = FLinearColor(0.8f, 0.3f, 0.3f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor NeutralColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
};
