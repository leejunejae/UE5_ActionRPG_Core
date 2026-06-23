// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Data/CombatData.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "Utils/AnimBoneDataRegistryRoot.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"
#include "UObject/NoExportTypes.h"
#include "AttackData.generated.h"

UENUM(BlueprintType)
enum class EAttackSourceType : uint8
{
	MainHand,
	OffHand,
	Custom
};

USTRUCT(BlueprintType)
struct FAttackTraceSource
{
	GENERATED_BODY()

public:
	UPROPERTY() USceneComponent* TraceComponent = nullptr; // 무기 메시 or 캐릭터 메시
	UPROPERTY() float Radius = 0.f;

	bool IsValid() { return TraceComponent ? true : false; }
	// 필요하면: 추가 소켓들, 오프셋, 채널, 트레이스 프로파일 등
};

USTRUCT(BlueprintType)
struct FAttackDamageSource
{
	GENERATED_BODY()

public:
	UPROPERTY() float AttackRating = 0.f;   // (무기공격력 * 스탯보정)
	UPROPERTY() float PoiseRating = 0.f;
	UPROPERTY() float StanceRating = 0.f;
};

USTRUCT(BlueprintType)
struct FBaseAttackData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName SectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EHitResponse Response;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EDamageType DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EAttackSourceType AttackSource;

	// ── 속성 ──────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EElementalType ElementType = EElementalType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float ElementalBuildup = 0.f;

	// ── 배율 ──────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PoiseDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float StanceDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float StaminaCostMultiplier = 1.0f;

	// ── 판정 플래그 ────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanBlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanParried = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanAvoid = false;

	bool operator==(const FName& Other) const
	{
		return SectionName == Other;
	}
};

USTRUCT(Atomic, BlueprintType)
struct FAttackContext
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName AttackName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UAnimMontage* Anim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FBaseAttackData> AttackDetail;

	bool IsValid() const
	{
		return Anim != nullptr && !AttackDetail.IsEmpty();
	}

	inline bool operator==(const FAttackContext& Other) const
	{
		return AttackName == Other.AttackName;
	}
};

uint32 GetTypeHash(const FAttackContext& AttackType);

USTRUCT(Atomic, BlueprintType)
struct FAttackContextSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSet<FAttackContext> Contexts;
};

UCLASS()
class UE5PROJECT_API UAttackData : public UObject
{
	GENERATED_BODY()
	
};
