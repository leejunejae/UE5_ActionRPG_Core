// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "CharacterStatData.generated.h"

/* ============================================================
 *  Attribute - 플레이어 캐릭터 영구 능력치
 * ============================================================ */
UENUM(BlueprintType)
enum class EAttributeType : uint8
{
	Vitality,		// 생명력 — HP Max
	Endurance,		// 지구력 — Stamina Max
	Mentality,		// 정신력 — Focus Max
	Strength,		// 근력   — 공격력 보정, 강인도 Max, 장비 하중 Max, 중량 무기 요구치
	Dexterity,		// 민첩   — 공격력 보정, 경량 무기 요구치, Stamina 회복 속도
	Affinity		// 자연친화 — 자연물 계열 무기 요구치, 스킬 트리 관련 (추후 확장)
};

// ── 생명력 ──────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FAttribute_Vitality : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float HP = 0.f;
};

// ── 지구력 ──────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FAttribute_Endurance : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float Stamina = 0.f;
};

// ── 정신력 ──────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FAttribute_Mentality : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

	UPROPERTY(EditAnywhere)
		float Focus = 0.f;
};

// ── 근력 ────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FAttribute_Strength : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

		// 물리 공격력 보정계수 (무기 기본 공격력에 곱해지는 값)
		// 무기마다 근력/민첩 보정계수 비율이 다름
	UPROPERTY(EditAnywhere) 
		float PhysicalAttackBonus = 0.f;

		// 강인도 Max (경직 트리거 임계값)
	UPROPERTY(EditAnywhere) 
		float PoiseMax = 0.f;

		// 장비 하중 Max
	UPROPERTY(EditAnywhere) 
		float EquipLoadMax = 0.f;
};

// ── 민첩 ────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FAttribute_Dexterity : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		int32 Level;

		// 물리 공격력 보정계수 (근력과 동일 구조, 무기별 비율로 혼합)
	UPROPERTY(EditAnywhere) 
		float PhysicalAttackBonus = 0.f;

		// Stamina 초당 회복 보너스
	UPROPERTY(EditAnywhere) 
		float StaminaRegenBonus = 0.f;
};

// ── 자연친화 ────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FAttribute_Affinity : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) int32 Level;

	// 자연물 계열 무기 요구치 판정에 사용
	// 스킬 트리 관련 수치는 스킬 시스템 확정 후 추가 예정
};

USTRUCT(BlueprintType)
struct FCharacterAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Vitality = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Endurance = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Mentality = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Strength = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Dexterity = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Affinity = 10;

	FCharacterAttributes operator+(const FCharacterAttributes& Other) const
	{
		return {
			Vitality + Other.Vitality,
			Endurance + Other.Endurance,
			Mentality + Other.Mentality,
			Strength + Other.Strength,
			Dexterity + Other.Dexterity,
			Affinity + Other.Affinity
		};
	}

	float GetRequirementAttributeRate(const FCharacterAttributes& Requirement) const;
};

/* ============================================================
 *  FPlayerCombatStats — 전투 보정 수치 (특성에서 계산된 결과값)
 *  PlayerStatComponent가 InitializeStats 후 보관
 * ============================================================ */
USTRUCT(BlueprintType)
struct FPlayerCombatStats
{
	GENERATED_BODY()

	// 근력 기반 물리 공격력 보정계수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) 
	float StrengthAttackBonus = 0.f;

	// 민첩 기반 물리 공격력 보정계수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) 
	float DexterityAttackBonus = 0.f;

	// Stamina 초당 회복 보너스 (민첩에서)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) 
	float StaminaRegenBonus = 0.f;
};

/* ============================================================
 *  Resources - 런타임 스탯
 * ============================================================ */

// 스탯 소모를 나타내기 위한 리소스 타입 정의(Enemy가 사용하는 Stance의 경우 표시되지 않으므로 제외)
UENUM(BlueprintType)
enum class EResourceStatType : uint8
{
	Health,
	Stamina,
	Focus
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
		FPlayerCombatStats CombatStats;
};

USTRUCT(BlueprintType)
struct FNPCStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FCharacterStats BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FResourceStat Stance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PhysicalAttackPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MagicAttackPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PoiseAttackPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float StaminaAttackPower = 0.f;

public:
	FORCEINLINE float GetStance() const { return Stance.Current; }
	FORCEINLINE float GetMaxStance() const { return Stance.Max; }
};

UCLASS()
class UE5PROJECT_API UCharacterStatData : public UObject
{
	GENERATED_BODY()
	
};
