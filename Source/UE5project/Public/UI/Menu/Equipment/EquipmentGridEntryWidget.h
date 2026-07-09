// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUserWidget.h"
#include "EquipmentGridEntryWidget.generated.h"


class UButton;
class UImage;
class UWidget;

DECLARE_DELEGATE_OneParam(FOnEntryClicked, FName);
DECLARE_DELEGATE_OneParam(FOnEntryDoubleClicked, FName);

UCLASS(Abstract)
class UE5PROJECT_API UEquipmentGridEntryWidget : public UBaseUserWidget
{
	GENERATED_BODY()
	
public:
	FOnEntryClicked OnEntryClicked;
	FOnEntryDoubleClicked OnEntryDoubleClicked;

	void InitEntry(FName InKey, UTexture2D* Icon, bool bIsEquipped, bool bIsSelected);
	void SetSelected(bool bSelected);
	void SetEquipped(bool bEquipped);

	FORCEINLINE FName GetItemKey() const { return ItemKey; }

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnSelectedChanged(bool bSelected);

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Root;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Icon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Indicator_Equipped;

private:
	UFUNCTION() void HandleClicked();

	float LastClickTime = -1.f;
	static constexpr float DoubleClickThreshold = 0.3f;

	FName ItemKey = NAME_None;
};
