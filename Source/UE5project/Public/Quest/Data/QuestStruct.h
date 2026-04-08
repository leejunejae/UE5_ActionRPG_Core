// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "QuestStruct.generated.h"

UENUM(BlueprintType)
enum class QuestState : uint8
{
	Invalid UMETA(DisplayName = "Invalid"),
	Activated UMETA(DisplayName = "Activated"),
	Completed UMETA(DisplayName = "Completed"),
	Executable UMETA(DisplayName = "Executable"),
	Precondition UMETA(DisplayName = "Precondition"),
};

USTRUCT(Atomic, BlueprintType)
struct FRewardStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 Cash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Exp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FName, int32> Items;
};

USTRUCT(Atomic, BlueprintType)
struct FObjectiveDataStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 RequiredAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 CurAmount;

	inline bool operator==(const FObjectiveDataStruct& Other) const
	{
		return ID == Other.ID;
	}
};

uint32 GetTypeHash(const FObjectiveDataStruct& ObjectiveData);

USTRUCT(Atomic, BlueprintType)
struct FQuestDataStruct : public FTableRowBase
{
	GENERATED_BODY()
		
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSet<FObjectiveDataStruct> ObjectiveList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRewardStruct Reward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FName> PreQuests;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FName> LinkedQuests;
};

UCLASS()
class UE5PROJECT_API AQuestStruct : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AQuestStruct();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
