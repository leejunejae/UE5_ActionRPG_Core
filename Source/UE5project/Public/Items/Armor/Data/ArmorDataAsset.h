// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/Armor/Data/ArmorData.h"
#include "ArmorDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UArmorDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 이 에셋이 속하는 부위
	// EquipmentComponent가 슬롯 자동 판단에 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EArmorSlot ArmorSlot = EArmorSlot::Chest;

	// 이 부위의 Skeletal Mesh
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USkeletalMesh> Mesh;

	// UI용 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon;

	// 표시 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	// 표시 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;
};
