// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WidgetInterface.h"
#include "InteractWidget.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UInteractWidget : public UWidgetInterface
{
	GENERATED_BODY()
	
public:
	UInteractWidget(const FObjectInitializer& ObjectInitializer);

private:
	UPROPERTY(meta = (BindWidget))
		class UEditableText* InteractText;

	UPROPERTY(meta = (BindWidget))
		class UEditableTextBox* InteractKey;
};
