// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "MiscItemData.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Consumable,
	Material,
	QuestItem
};

// 소모품/재료/퀘스트아이템 공용 DataTable Row
USTRUCT(BlueprintType)
struct FMiscItemInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EItemCategory Category = EItemCategory::Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsStackable = true;
};

UCLASS()
class UE5PROJECT_API UMiscItemData : public UObject
{
	GENERATED_BODY()
	
};
