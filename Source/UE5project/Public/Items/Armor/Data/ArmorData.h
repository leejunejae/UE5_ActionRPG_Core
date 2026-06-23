// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "ArmorData.generated.h"

/**
 * 
 */
class UArmorDataAsset;

UENUM(BlueprintType)
enum class EArmorSlot : uint8
{
	Head    UMETA(DisplayName = "Head"),
	Chest   UMETA(DisplayName = "Chest"),
	Hands   UMETA(DisplayName = "Hands"),
	Legs    UMETA(DisplayName = "Legs"),
};

USTRUCT(Atomic, BlueprintType)
struct FArmorPieceInfo : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 외형 정의 에셋 (장착 시 동기 로드)
	// ArmorDataAsset.ArmorSlot 으로 슬롯 자동 판단
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UArmorDataAsset> ArmorDefinition;

	// ── 방어력 ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DefenseValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float MagicDefenseValue = 0.f;

	// ── 상태이상 저항력 ──────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float FireResistance = 0.f;		// 화염 저항 (화상)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float FrostResistance = 0.f;	// 냉기 저항 (동상)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float PoisonResistance = 0.f;	// 독 저항 (중독)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float BleedResistance = 0.f;	// 출혈 저항

	// ── 무게 ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float WeightValue = 0.f;

public:
	FArmorPieceInfo() {}
};

UCLASS()
class UE5PROJECT_API UArmorData : public UObject
{
	GENERATED_BODY()
	
};
