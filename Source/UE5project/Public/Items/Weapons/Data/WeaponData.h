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
struct FWeaponAttributeRequirement
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

/* ============================================================
 *  요구 스탯 충족도 브레이크다운 — 장비 탭 UI 표시용
 * ============================================================ */
USTRUCT(BlueprintType)
struct FWeaponRequirementRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 RequiredValue = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentValue = 0;

	// CurrentValue / RequiredValue, 0~1 clamp (RequiredValue가 0이면 1.0)
	UPROPERTY(BlueprintReadOnly)
	float FulfillRatio = 1.f;

	UPROPERTY(BlueprintReadOnly)
	EWeaponGrade Grade = EWeaponGrade::None;

	// (해당 스탯의 AttackBonus 계수) × GradeMultiplier
	UPROPERTY(BlueprintReadOnly)
	float AppliedAttackValue = 0.f;

	// CalcAttributeAttackBonus에서 세 스탯 중 최댓값으로 채택된 스탯인지
	UPROPERTY(BlueprintReadOnly)
	bool bIsAdopted = false;
};

USTRUCT(BlueprintType)
struct FWeaponRequirementBreakdown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FWeaponRequirementRow Strength;

	UPROPERTY(BlueprintReadOnly)
	FWeaponRequirementRow Dexterity;

	UPROPERTY(BlueprintReadOnly)
	FWeaponRequirementRow Affinity;
};

USTRUCT(Atomic, BlueprintType)
struct FWeaponSetsInfo : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<UWeaponDataAsset> WeaponDefenition;

		// 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float AttackPower; 

		// 강인도 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float PoisePower; 

		// 자세 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float StancePower;

		// 스태미나 소모값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float StaminaCost; 

		// 공격 시 추가되는 강인도 보너스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float PoiseBonus; 

		// 가드시 경감률(가드시 데미지 감소율)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"))
		float GuardNegation; 

		// 가드 강도(값만큼 퍼센트로 들어온 스태미나 소모율 감소)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"))
		float GuardBoost; 

		// 무게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float WeightValue = 0.0f; 

		// 능력치 요구값(무기 성능을 발휘하기 위한 능력치 제한)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FWeaponAttributeRequirement RequiredAttributes;

		// 무기 보정치(능력치에 따라 무기 보정)
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


static FWeaponRequirementBreakdown CalculateWeaponRequirementBreakdown(
	const FWeaponSetsInfo* Weapon,
	const FCharacterAttributes& CurrentAttrs,
	float StrengthBonus, float DexterityBonus, float AffinityBonus)
{
	FWeaponRequirementBreakdown Out;
	if (!Weapon) return Out;

	auto BuildRow = [](int32 CurrentValue, int32 RequiredValue, EWeaponGrade Grade, float AttackBonus) -> FWeaponRequirementRow
		{
			FWeaponRequirementRow Row;
			Row.RequiredValue = RequiredValue;
			Row.CurrentValue = CurrentValue;
			Row.FulfillRatio = RequiredValue > 0
				? FMath::Clamp((float)CurrentValue / RequiredValue, 0.f, 1.f) : 1.f;
			Row.Grade = Grade;
			Row.AppliedAttackValue = AttackBonus * GetGradeMultiplier(Grade);
			return Row;
		};

	Out.Strength = BuildRow(CurrentAttrs.Strength, Weapon->RequiredAttributes.Strength, Weapon->Scaling.StrengthGrade, StrengthBonus);
	Out.Dexterity = BuildRow(CurrentAttrs.Dexterity, Weapon->RequiredAttributes.Dexterity, Weapon->Scaling.DexterityGrade, DexterityBonus);
	Out.Affinity = BuildRow(CurrentAttrs.Affinity, Weapon->RequiredAttributes.Affinity, Weapon->Scaling.AffinityGrade, AffinityBonus);

	FWeaponRequirementRow* Rows[3] = { &Out.Strength, &Out.Dexterity, &Out.Affinity };
	FWeaponRequirementRow* MaxRow = Rows[0];
	for (FWeaponRequirementRow* R : Rows)
	{
		if (R->AppliedAttackValue > MaxRow->AppliedAttackValue) MaxRow = R;
	}
	MaxRow->bIsAdopted = true;

	return Out;
}

UCLASS()
class UE5PROJECT_API UWeaponData : public UObject
{
	GENERATED_BODY()

};
