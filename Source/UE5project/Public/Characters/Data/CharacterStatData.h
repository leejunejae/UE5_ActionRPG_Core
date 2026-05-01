// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "CharacterStatData.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EAttributeType : uint8
{
	Vitality,
	Endurance,
	Mentality,
	Strength,
	Dexterity,
	Intelligence,
	Vigor
};

USTRUCT(BlueprintType)
struct FAttribute_Vitality : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float HP = 0.f;
};

USTRUCT(BlueprintType)
struct FAttribute_Endurance : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float Stamina = 0.f;
};

USTRUCT(BlueprintType)
struct FAttribute_Mentality : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float FP = 0.f;
};

USTRUCT(BlueprintType)
struct FAttribute_Strength : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float PhysicalDefense = 0.f;
};

USTRUCT(BlueprintType)
struct FAttribute_Dexterity : public FTableRowBase
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float Poise = 0.f;

	UPROPERTY(EditAnywhere)
		float Evasion = 0.f;
};

USTRUCT(BlueprintType)
struct FAttribute_Intelligence : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float MagicDefense = 0.f;
};

USTRUCT(BlueprintType)
struct FAttribute_Vigor : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float EquipLoad = 0.f;

	UPROPERTY(EditAnywhere)
		float Resistance = 0.f;
};

USTRUCT(BlueprintType)
struct FCharacterAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Vitality = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Endurance = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Mentality = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Strength = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Dexterity = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Intelligence = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Vigor = 10;

	// 스탯 합산, 비교, 증감 같은 유틸 함수도 여기 포함 가능
	FCharacterAttributes operator+(const FCharacterAttributes& Other) const
	{
		return {
			Vitality + Other.Vitality,
			Endurance + Other.Endurance,
			Mentality + Other.Mentality,
			Strength + Other.Strength,
			Dexterity + Other.Dexterity,
			Intelligence + Other.Intelligence,
			Vigor + Other.Vigor
		};
	}

	float GetRequirementAttributeRate(const FCharacterAttributes& Requirement) const;
};

USTRUCT(BlueprintType)
struct FResourceStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) 
		float Current;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) 
		float Max;

public:
	void InitResource(float Value)
	{
		Current = Value;
		Max = Value;
	}
};

USTRUCT(BlueprintType)
struct FCharacterStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FResourceStat Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FResourceStat Poise;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PhysicalDefense = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MagicDefense = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Resistance = 0;

public:
	FORCEINLINE float GetHealth() const { return Health.Current; }
	FORCEINLINE float GetMaxHealth() const { return Health.Max; }
	FORCEINLINE float GetHealthPercent() const
	{
		return (GetMaxHealth() > KINDA_SMALL_NUMBER) ? (GetHealth() / GetMaxHealth()) : 0.0f;
	}

	FORCEINLINE float GetPoise() const { return Poise.Current; }
	FORCEINLINE  float GetMaxPoise() const { return Poise.Max; }
};

USTRUCT(BlueprintType)
struct FPlayerStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FCharacterStats BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FResourceStat Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FResourceStat Focus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FResourceStat EquipLoad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Evasion;
};

USTRUCT(BlueprintType)
struct FNPCStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FCharacterStats BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float AttackPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FResourceStat Stance;

public:
	FORCEINLINE float GetStance() const { return Stance.Current; }
	FORCEINLINE float GetMaxStance() const { return Stance.Max; }
};

UCLASS()
class UE5PROJECT_API UCharacterStatData : public UObject
{
	GENERATED_BODY()
	
};
