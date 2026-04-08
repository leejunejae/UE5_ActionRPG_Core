// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "Characters/Data/CharacterStatData.h"
#include "WeaponData.generated.h"

class UWeaponDataAsset;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "None"),
	SwordAndShield UMETA(DisplayName = "SwordAndShield"),
	LongSword UMETA(DisplayName = "LongSword"),
	GreatSword UMETA(DisplayName = "GreatSword"),
	SpearAndShield UMETA(DisplayName = "SpearAndShield"),
	Knuckles UMETA(DisplayName = "Knuckles"),
};

USTRUCT(BlueprintType)
struct FWeaponStatRequirement
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Strength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Dexterity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Intelligence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Vigor = 0;

	FCharacterAttributes ToCharacterStats() const
	{
		FCharacterAttributes Stats;
		Stats.Strength = Strength;
		Stats.Dexterity = Dexterity;
		Stats.Intelligence = Intelligence;
		Stats.Vigor = Vigor;
		return Stats;
	}
};

USTRUCT(Atomic, BlueprintType)
struct FWeaponSetsInfo : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<UWeaponDataAsset> WeaponDefenition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float AttackPower; // 공격력

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float PoisePower; // 경직치

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float StancePower; // 스탠스 데미지

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float StaminaCost; // 스태미나 소모값

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float PoiseBonus; // 공격 시 추가되는 경직 보너스

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float GuardNegation; // 가드시 경감률(가드시 데미지 감소율)

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float GuardBoost; // 가드 강도(값만큼 퍼센트로 들어온 스태미나 소모율 감소)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FWeaponStatRequirement RequiredStats;

public:
	FWeaponSetsInfo(){}
};

UCLASS()
class UE5PROJECT_API UWeaponData : public UObject
{
	GENERATED_BODY()

};
