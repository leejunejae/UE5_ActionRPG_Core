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
	Death UMETA(DislplayName = "Death"),
	DeathLarge UMETA(DislplayName = "DeathLarge"),
};
/*
* 공격 데이터 자료형
*/
UENUM(BlueprintType)
enum class EStatusEffect : uint8
{
	Bleeding UMETA(DisplayName = "Bleeding"),
	Darkness UMETA(DisplayName = "Darkness"),
	SlowDown UMETA(DisplayName = "SlowDown"),
	Weakness UMETA(DisplayName = "Weakness"),
	Restraint UMETA(DisplayName = "Restraint"),
};

USTRUCT(BlueprintType)
struct FStatusEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EStatusEffect StatusEffect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Amount;
};

USTRUCT(BlueprintType)
struct FAttackRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Damage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float StanceDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PoiseDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EHitResponse Response;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EDamageType AttackType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector HitPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString HitPointName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanBlocked;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanParried;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanAvoid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FStatusEffect> StatusEffectList;

public:
	FAttackRequest() {}
	FAttackRequest(
		float InDamage,
		float InStanceDamage,
		float InPoiseDamage,
		EHitResponse InResponse,
		FVector InHitPoint,
		FString InHitPointName,
		bool InCanBlocked,
		bool InCanParried,
		bool InCanAvoid,
		const TArray<FStatusEffect>& InStatusEffectList
		)
		: Damage(InDamage)
		, StanceDamage(InStanceDamage)
		, PoiseDamage(InPoiseDamage)
		, Response(InResponse)
		, HitPoint(InHitPoint)
		, HitPointName(InHitPointName)
		, CanBlocked(InCanBlocked)
		, CanParried(InCanParried)
		, CanAvoid(InCanAvoid)
		, StatusEffectList(InStatusEffectList)
	{}
};

UCLASS()
class UE5PROJECT_API UCombatData : public UObject
{
	GENERATED_BODY()

};
