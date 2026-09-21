// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Characters/Data/CharacterStatData.h"
#include "WeaponData.generated.h"

class UWeaponDataAsset;
class UOffHandWeaponDataAsset;

/** 물리적인 무기 계열. 장착 조합이나 사용 애니메이션을 나타내지 않는다. */
UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	None UMETA(DisplayName = "None"),
	Sword UMETA(DisplayName = "Sword"),
	Dagger UMETA(DisplayName = "Dagger"),
	GreatSword UMETA(DisplayName = "Great Sword"),
	Spear UMETA(DisplayName = "Spear"),
	Axe UMETA(DisplayName = "Axe"),
	GreatAxe UMETA(DisplayName = "Great Axe"),
	Mace UMETA(DisplayName = "Mace"),
	Hammer UMETA(DisplayName = "Hammer"),
	Bow UMETA(DisplayName = "Bow"),
	Crossbow UMETA(DisplayName = "Crossbow"),
	Staff UMETA(DisplayName = "Staff"),
	Shield UMETA(DisplayName = "Shield"),
	FistWeapon UMETA(DisplayName = "Fist Weapon"),
	ThrowingWeapon UMETA(DisplayName = "Throwing Weapon"),
};

/** 무기 자체가 허용하는 파지 방식. */
UENUM(BlueprintType)
enum class EWeaponGripType : uint8
{
	OneHanded UMETA(DisplayName = "One Handed"),
	TwoHanded UMETA(DisplayName = "Two Handed"),
	Versatile UMETA(DisplayName = "Versatile"),
};

/** 현재 캐릭터가 실제로 사용 중인 파지 상태. */
UENUM(BlueprintType)
enum class EWeaponGripMode : uint8
{
	OneHanded UMETA(DisplayName = "One Handed"),
	TwoHanded UMETA(DisplayName = "Two Handed"),
};

/** 장착 상태를 유지한 무기가 현재 손에 있는지 몸에 보관되어 있는지 나타낸다. */
UENUM(BlueprintType)
enum class EWeaponPresentationState : uint8
{
	Drawn UMETA(DisplayName = "Drawn"),
	Holstered UMETA(DisplayName = "Holstered"),
};

UENUM(BlueprintType)
enum class EWeaponTraceShape : uint8
{
	Capsule UMETA(DisplayName = "Capsule"),
	Box UMETA(DisplayName = "Box"),
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

USTRUCT(BlueprintType)
struct FWeaponStatsRow : public FTableRowBase
{
	GENERATED_BODY()

public:
		// 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float AttackPower = 0.0f;

		// 강인도 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float PoisePower = 0.0f;

		// 자세 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float StancePower = 0.0f;

		// 스태미나 소모값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float StaminaCost = 0.0f;

		// 공격 시 추가되는 강인도 보너스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float PoiseBonus = 0.0f;

		// 가드시 경감률(가드시 데미지 감소율)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"))
		float GuardNegation = 0.0f;

		// 가드 강도(값만큼 퍼센트로 들어온 스태미나 소모율 감소)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"))
		float GuardBoost = 0.0f;

		// 무게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
		float WeightValue = 0.0f; 

		// 능력치 요구값(무기 성능을 발휘하기 위한 능력치 제한)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FWeaponAttributeRequirement RequiredAttributes;

		// 무기 보정치(능력치에 따라 무기 보정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FWeaponScaling Scaling;

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

/** 주무기 데이터 테이블 행. */
USTRUCT(Atomic, BlueprintType)
struct FWeaponSetsInfo : public FWeaponStatsRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWeaponDataAsset> WeaponDefenition;
};

/** 보조무기 데이터 테이블 행. 모든 보조무기는 주무기와 동일한 공격 스탯을 갖는다. */
USTRUCT(Atomic, BlueprintType)
struct FOffHandWeaponSetsInfo : public FWeaponStatsRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UOffHandWeaponDataAsset> WeaponDefinition;
};


static FWeaponRequirementBreakdown CalculateWeaponRequirementBreakdown(
	const FWeaponStatsRow* Weapon,
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
