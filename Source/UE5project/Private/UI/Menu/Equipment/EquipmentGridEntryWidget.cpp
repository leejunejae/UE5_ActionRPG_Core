// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Menu/Equipment/EquipmentGridEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Widget.h"

#include "Utils/CoreLog.h"

void UEquipmentGridEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Btn_Root) Btn_Root->OnClicked.AddDynamic(this, &UEquipmentGridEntryWidget::HandleClicked);
}

void UEquipmentGridEntryWidget::InitEntry(FName InKey, UTexture2D* Icon, bool bIsEquipped, bool bIsSelected)
{
	ItemKey = InKey;

	if (Image_Icon)
	{
		if (Icon)
		{
			Image_Icon->SetBrushFromTexture(Icon);
			Image_Icon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Image_Icon->SetVisibility(ESlateVisibility::Collapsed);   // 빈 슬롯: 이미지 자체를 숨김
		}
	}

	SetEquipped(bIsEquipped);
	SetSelected(bIsSelected);
}

void UEquipmentGridEntryWidget::SetSelected(bool bSelected)
{
	OnSelectedChanged(bSelected);
}

void UEquipmentGridEntryWidget::SetEquipped(bool bEquipped)
{
	if (Indicator_Equipped) Indicator_Equipped->SetVisibility(bEquipped ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UEquipmentGridEntryWidget::HandleClicked()
{
	if (ItemKey == NAME_None) return;
	
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (LastClickTime > 0.f && (Now - LastClickTime) <= DoubleClickThreshold)
	{
		OnEntryDoubleClicked.ExecuteIfBound(ItemKey);
		LastClickTime = -1.f;   // 연속 트리플클릭 등이 두 번의 더블클릭으로 안 잡히게 리셋
	}
	else
	{
		OnEntryClicked.ExecuteIfBound(ItemKey);
		LastClickTime = Now;
	}
}