// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatData.generated.h"

// 
UENUM(BlueprintType)
enum class EDamageType : uint8
{
	PhysicalDamage UMETA(DisplayName = "PhysicalDamage"),
	MagicalDamage UMETA(DisplayName = "MagicalDamage"),
	TrueDamage UMETA(DisplayName = "TrueDamage"),
};

UENUM(BlueprintType)
enum class EStatChangeType : uint8
{
	Damage UMETA(DisplayName = "Damage"),
	Restore UMETA(DisplayName = "Restore"),
	Heal UMETA(DisplayName = "Heal"),
};

// 공격 데이터가 지정하는 직접 피격 반응의 강도.
// 가드 및 자세 붕괴 반응은 판정 결과에서 ECombatReaction으로 생성된다.
UENUM(BlueprintType)
enum class EHitDamageLevel : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Flinch = 1 UMETA(DisplayName = "Flinch"),
	KnockBack = 2 UMETA(DisplayName = "KnockBack"),
	KnockDown = 3 UMETA(DisplayName = "KnockDown"),
};

// 공격 해결 후 실제로 재생할 전투 반응.
// 공격의 적중/회피/가드/패리 여부는 EHitOutcome이 별도로 표현한다.
UENUM(BlueprintType)
enum class ECombatReaction : uint8
{
	None UMETA(DisplayName = "None"),
	Flinch UMETA(DisplayName = "Flinch"),
	KnockBack UMETA(DisplayName = "KnockBack"),
	KnockDown UMETA(DisplayName = "KnockDown"),
	HitAir UMETA(DisplayName = "HitAir"),
	GuardHit UMETA(DisplayName = "GuardHit"),
	GuardHitHeavy UMETA(DisplayName = "GuardHitHeavy"),
	GuardBreak UMETA(DisplayName = "GuardBreak"),
	StanceBreak UMETA(DisplayName = "StanceBreak"),
};

FORCEINLINE ECombatReaction ToCombatReaction(EHitDamageLevel DamageLevel)
{
	switch (DamageLevel)
	{
	case EHitDamageLevel::Flinch: return ECombatReaction::Flinch;
	case EHitDamageLevel::KnockBack: return ECombatReaction::KnockBack;
	case EHitDamageLevel::KnockDown: return ECombatReaction::KnockDown;
	default: return ECombatReaction::None;
	}
}

// 공격이 방어자에게 어떻게 해결되었는지를 나타낸다.
// 실제로 재생할 애니메이션 반응(ECombatReaction)과 분리해서 사용한다.
UENUM(BlueprintType)
enum class EHitOutcome : uint8
{
	Hit UMETA(DisplayName = "Hit"),
	Avoided UMETA(DisplayName = "Avoided"),
	Blocked UMETA(DisplayName = "Blocked"),
	Parried UMETA(DisplayName = "Parried"),
};

/* ============================================================
 *  EElementalType — 속성 종류
 *
 *  DamageType(물리/마법/고정)과 독립적으로 존재.
 *  예) 날카로운 검 = PhysicalDamage + Bleed 스택
 *      화염구     = MagicalDamage  + Fire  스택
 *      순수 타격  = PhysicalDamage + None  (속성 없음)
 *
 *  스택 처리는 추후 StatusEffectComponent에서 담당.
 *  FAttackRequest.ElementalBuildup 값을 읽어 피격 측에 누적.
 * ============================================================ */
UENUM(BlueprintType)
enum class EElementalType : uint8
{
	None    UMETA(DisplayName = "None"),    // 속성 없음
	Fire    UMETA(DisplayName = "Fire"),    // 화염 → 화상
	Frost   UMETA(DisplayName = "Frost"),   // 냉기 → 동상
	Poison  UMETA(DisplayName = "Poison"),  // 독   → 중독
	Bleed   UMETA(DisplayName = "Bleed"),   // 출혈
};

USTRUCT(BlueprintType)
struct FAttackRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StanceDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PoiseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitDamageLevel DamageLevel = EHitDamageLevel::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDamageType AttackType = EDamageType::PhysicalDamage;

	// ── 속성 ──────────────────────────────────────────────────
	// ElementType: 이 공격이 쌓는 속성 종류
	// ElementalBuildup: 이 공격 한 번이 누적시키는 스택량
	// 추후 StatusEffectComponent가 피격 측 저항력과 비교해 상태이상 판정
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EElementalType ElementType = EElementalType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ElementalBuildup = 0.f;

	// ── 피격 위치 ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HitPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HitPointName;

	// 가드/패리 방향 판정용 공격 주체. HitPoint는 무기 접촉 위치로 계속 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> AttackCauser = nullptr;

	// ── 판정 플래그 ────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanBlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanParried = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanAvoid = false;

public:
	FAttackRequest() {}

	FAttackRequest(
		float InDamage,
		float InStanceDamage,
		float InPoiseDamage,
		EHitDamageLevel InDamageLevel,
		EDamageType InAttackType,
		EElementalType InElementType,
		float InElementalBuildup,
		FVector InHitPoint,
		FString InHitPointName,
		bool InCanBlocked,
		bool InCanParried,
		bool InCanAvoid,
		AActor* InAttackCauser = nullptr)
		: Damage(InDamage)
		, StanceDamage(InStanceDamage)
		, PoiseDamage(InPoiseDamage)
		, DamageLevel(InDamageLevel)
		, AttackType(InAttackType)
		, ElementType(InElementType)
		, ElementalBuildup(InElementalBuildup)
		, HitPoint(InHitPoint)
		, HitPointName(InHitPointName)
		, AttackCauser(InAttackCauser)
		, CanBlocked(InCanBlocked)
		, CanParried(InCanParried)
		, CanAvoid(InCanAvoid)
	{
	}
};

USTRUCT(BlueprintType)
struct FHitResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EHitOutcome Outcome = EHitOutcome::Hit;

	UPROPERTY(BlueprintReadOnly)
	ECombatReaction Reaction = ECombatReaction::None;

	UPROPERTY(BlueprintReadOnly)
	float HitAngle = 0.0f;

	FHitResolution() = default;
	FHitResolution(EHitOutcome InOutcome, ECombatReaction InReaction, float InHitAngle)
		: Outcome(InOutcome), Reaction(InReaction), HitAngle(InHitAngle)
	{
	}
};

UCLASS()
class UE5PROJECT_API UCombatData : public UObject
{
	GENERATED_BODY()

};
