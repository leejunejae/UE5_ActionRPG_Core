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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DamageMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PoiseDamageMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float StanceDamageMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanBlocked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanParried;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanAvoid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FStatusEffect> StatusEffectList;

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
