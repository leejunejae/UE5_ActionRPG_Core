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

// 피격 유형 결정
UENUM(BlueprintType)
enum class EHitResponse : uint8
{
	None UMETA(DisplayName = "None"),
	NoStagger UMETA(DisplayName = "NoStagger"),
	Flinch UMETA(DisplayName = "Flinch"),
	KnockBack UMETA(DisplayName = "KnockBack"),
	KnockDown UMETA(DislplayName = "KnockDown"),
	Stun UMETA(DislplayName = "Stun"),
	HitAir UMETA(DislplayName = "HitAir"),
	Block UMETA(DisplayName = "Block"),
	BlockLarge UMETA(DisplayName = "BlockLarge"),
	BlockBreak UMETA(DisplayName = "BlockBreak"),
	BlockStun UMETA(DisplayName = "BlockStun"),
	Parry UMETA(DisplayName = "Parry"),
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
	EHitResponse Response = EHitResponse::None;

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
		EHitResponse InResponse,
		EDamageType InAttackType,
		EElementalType InElementType,
		float InElementalBuildup,
		FVector InHitPoint,
		FString InHitPointName,
		bool InCanBlocked,
		bool InCanParried,
		bool InCanAvoid)
		: Damage(InDamage)
		, StanceDamage(InStanceDamage)
		, PoiseDamage(InPoiseDamage)
		, Response(InResponse)
		, AttackType(InAttackType)
		, ElementType(InElementType)
		, ElementalBuildup(InElementalBuildup)
		, HitPoint(InHitPoint)
		, HitPointName(InHitPointName)
		, CanBlocked(InCanBlocked)
		, CanParried(InCanParried)
		, CanAvoid(InCanAvoid)
	{
	}
};

UCLASS()
class UE5PROJECT_API UCombatData : public UObject
{
	GENERATED_BODY()

};
