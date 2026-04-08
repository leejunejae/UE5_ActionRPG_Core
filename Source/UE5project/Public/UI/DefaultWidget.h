// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WidgetInterface.h"
#include "DefaultWidget.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UDefaultWidget : public UWidgetInterface
{
	GENERATED_BODY()
	
public:
	UDefaultWidget(const FObjectInitializer& ObjectInitializer);

	// �÷��̾� �޴� ���� ����
	void SetUp();
	virtual bool Initialize();

private:
#pragma region PlayerDefaultUI
	UPROPERTY(meta = (BindWidget))
		class UProgressBar* HP_ProgressBar;

	UPROPERTY(meta = (BindWidget))
		class UProgressBar* Stamina_ProgressBar;
#pragma endregion
};
