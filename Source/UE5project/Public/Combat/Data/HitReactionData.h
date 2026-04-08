// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Combat/Data/CombatData.h"
#include "UObject/NoExportTypes.h"
#include "HitReactionData.generated.h"

UENUM(BlueprintType)
enum class EAnimLoopType : uint8
{
	Time UMETA(DisplayName="Time"),
	Condition UMETA(DisplayName="Condition")
};

USTRUCT(Atomic, BlueprintType)
struct FHitReactionRequest
{
	GENERATED_BODY()

public:
	EHitResponse Response;
	float HitAngle;
};


UENUM(BlueprintType)
enum class EHitPointVertical : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Up UMETA(DisplayName = "Up"),
	Down UMETA(DisplayName = "Down"),
};

UENUM(BlueprintType)
enum class EHitPointHorizontal : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
	Back UMETA(DisplayName = "Back"),
	Front UMETA(DisplayName = "Front"),
	FrontRight UMETA(DisplayName = "FrontRight"),
	FrontLeft UMETA(DisplayName = "FrontLeft"),
	BackRight UMETA(DisplayName = "BackRight"),
	BackLeft UMETA(DisplayName = "BackLeft"),
};

USTRUCT(Atomic, BlueprintType)
struct FHitReactionDetail
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName SectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EHitPointVertical HitPointVertical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EHitPointHorizontal HitPointHorizontal;

	inline bool operator==(const FHitReactionDetail& Other) const
	{
		return SectionName == Other.SectionName;
	}
};

uint32 GetTypeHash(const FHitReactionDetail& HitReactionDetail);

USTRUCT(Atomic, BlueprintType)
struct FHitReactionInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName HitReactionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TObjectPtr<UAnimMontage> Anim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSet<FHitReactionDetail> HitReactionDetail;

	bool IsValid() const
	{
		return Anim != nullptr && !HitReactionDetail.IsEmpty();
	}

	inline bool operator==(const FHitReactionInfo& Other) const
	{
		return HitReactionName == Other.HitReactionName;
	}
};

uint32 GetTypeHash(const FHitReactionInfo& Info);

USTRUCT(Atomic, BlueprintType)
struct FHitReactionInfoList
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSet<FHitReactionInfo> HitReactionInfo;
};

UCLASS()
class UE5PROJECT_API UHitReactionData : public UObject
{
	GENERATED_BODY()

};
