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

/* ============================================================
 *  특성 보정 등급
 *  AttackRating = (BasePower × PerformanceRatio)
 *               + (AttributeBonus × GradeMultiplier)
 * ============================================================ */
UENUM(BlueprintType)
enum class EWeaponGrade : uint8
{
	None	UMETA(DisplayName = "E"),	// 보정 없음  (×0.0)
	D		UMETA(DisplayName = "D"),	// (×0.2)
	C		UMETA(DisplayName = "C"),	// (×0.4)
	B		UMETA(DisplayName = "B"),	// (×0.6)
	A		UMETA(DisplayName = "A"),	// (×0.8)
	S		UMETA(DisplayName = "S"),	// (×1.0)
};

/* 등급 → 배율 변환 */
static float GetGradeMultiplier(EWeaponGrade Grade)
{
	switch (Grade)
	{
	case EWeaponGrade::S:    return 1.0f;
	case EWeaponGrade::A:    return 0.8f;
	case EWeaponGrade::B:    return 0.6f;
	case EWeaponGrade::C:    return 0.4f;
	case EWeaponGrade::D:    return 0.2f;
	case EWeaponGrade::None: return 0.0f;
	default:                 return 0.0f;
	}
}

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
		int32 Affinity = 0;

	FCharacterAttributes ToCharacterStats() const
	{
		FCharacterAttributes Stats;
		Stats.Strength = Strength;
		Stats.Dexterity = Dexterity;
		Stats.Affinity = Affinity;
		return Stats;
	}
};

/* ============================================================
 *  무기 특성 보정 등급
 *  모든 무기가 Strength/Dexterity/Affinity 등급을 각각 보유
 *  세 보정값 중 가장 높은 값을 최종 보정으로 적용
 *
 *  등급 조합으로 무기 정체성 결정:
 *    GreatSword    : Strength S, Dexterity D, Affinity E
 *    SwordAndShield: Strength D, Dexterity S, Affinity E
 *    LongSword     : Strength B, Dexterity B, Affinity E
 *    자연물 계열   : Strength C, Dexterity C, Affinity S
 * ============================================================ */

USTRUCT(BlueprintType)
struct FWeaponScaling
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponGrade StrengthGrade = EWeaponGrade::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponGrade DexterityGrade = EWeaponGrade::None;

	// 자연친화 보정은 스킬 시스템 확정 후 수치 채울 예정
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponGrade AffinityGrade = EWeaponGrade::None;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FWeaponScaling Scaling;

public:
	FWeaponSetsInfo(){}

	/**
	 * 세 특성 보정값 각각에 등급 배율을 곱한 뒤 가장 높은 값을 반환
	 * AffinityBonus는 스킬 시스템 확정 전까지 0.f 전달
	 */
	float CalcAttributeAttackBonus(float StrengthBonus, float DexterityBonus, float AffinityBonus = 0.f) const
	{
		const float StrResult = StrengthBonus * GetGradeMultiplier(Scaling.StrengthGrade);
		const float DexResult = DexterityBonus * GetGradeMultiplier(Scaling.DexterityGrade);
		const float AffResult = AffinityBonus * GetGradeMultiplier(Scaling.AffinityGrade);
		return FMath::Max3(StrResult, DexResult, AffResult);
	}
};