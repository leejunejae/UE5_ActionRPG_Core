// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Filters/CurveEditorFilterBase.h"
#include "CurveEditorSubtractionFilter.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EArithmeticType : uint8
{
	Addition UMETA(DisplayName = "Addition"),
	Subtraction UMETA(DisplayName = "Subtraction"),
	Division UMETA(DisplayName = "Division"),
	Multiplation UMETA(DisplayName = "Multiplation"),
};

UCLASS(DisplayName = "Arithmetic")
class UE5PROJECT_API UCurveEditorSubtractionFilter : public UCurveEditorFilterBase
{
	GENERATED_BODY()
	
public:
	UCurveEditorSubtractionFilter();

protected:
	virtual void ApplyFilter_Impl(TSharedRef<FCurveEditor> InCurveEditor, const TMap<FCurveModelID, FKeyHandleSet>& InKeysToOperateOn, TMap<FCurveModelID, FKeyHandleSet>& OutKeysToSelect) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
		EArithmeticType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
		float Value;
};
