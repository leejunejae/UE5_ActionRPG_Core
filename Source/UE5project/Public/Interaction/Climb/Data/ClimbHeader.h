// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveFloat.h"
#include "Characters/Data/IKData.h"
#include "UObject/NoExportTypes.h"
#include "ClimbHeader.generated.h"

UENUM(BlueprintType)
enum class EClimbPhase : uint8
{
	Enter_From_Bottom UMETA(DisplayName = "Enter_From_Bottom"),
	Enter_From_Top UMETA(DisplayName = "Enter_From_Top"),
	Idle UMETA(DisplayName = "Idle"),
	Idle_OneStep UMETA(DisplayName = "Idle_OneStep"),
	ClimbUp_Right UMETA(DisplayName = "ClimbUp_Right"),
	ClimbUp_Left UMETA(DisplayName = "ClimbUp_Left"),
	ClimbUp_OneStep UMETA(DisplayName = "ClimbUp_OneStep"),
	ClimbDown_Right UMETA(DisplayName = "ClimbDown_Right"),
	ClimbDown_Left UMETA(DisplayName = "ClimbDown_Left"),
	ClimbDown_OneStep UMETA(DisplayName = "ClimbDown_OneStep"),
	Exit_From_Bottom_Right UMETA(DisplayName = "Exit_From_Bottom_Right"),
	Exit_From_Bottom_Left UMETA(DisplayName = "Exit_From_Bottom_Left"),
	Exit_From_Top_Right UMETA(DisplayName = "Exit_From_Top_Right"),
	Exit_From_Top_Left UMETA(DisplayName = "Exit_From_Top_Left"),
};

UENUM(BlueprintType)
enum class EClimbStepSide : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
};

USTRUCT(BlueprintType)
struct FClimbCurveKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
		EClimbPhase Phase = EClimbPhase::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
		ELimbList Limb = ELimbList::Body;

	bool operator==(const FClimbCurveKey& Other) const
	{
		return Phase == Other.Phase && Limb == Other.Limb;
	}
};

FORCEINLINE uint32 GetTypeHash(const FClimbCurveKey& K)
{
	return HashCombine(uint32(K.Phase), uint32(K.Limb));
}

USTRUCT(BlueprintType)
struct FLadderClimbCurveSet_Map
{
	GENERATED_BODY()
	// SoftObjectPtr로 참조하면 필요할 때만 로드 → 요리/메모리 부담↓
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TMap<FClimbCurveKey, TSoftObjectPtr<UCurveVector>> Curves;
};

USTRUCT(Atomic, BlueprintType)
struct FNeighborInfo
{
	GENERATED_BODY()

	FGripNode1D* Neighbor;
	float Distance;

	inline bool operator==(const FNeighborInfo& Other) const
	{
		return Neighbor == Other.Neighbor;
	}
};

USTRUCT(BlueprintType)
struct FGripNode2D
{
	GENERATED_BODY()

	FVector Position;
	TArray<FName> Tag;
	TArray<FGripNode2D*> NeighborsUp;
	TArray<FGripNode2D*> NeighborsDown;
	TArray<FGripNode2D*> NeighborsRight;
	TArray<FGripNode2D*> NeighborsLeft;
};

USTRUCT(Atomic,BlueprintType)
struct FGripNode1D
{
	GENERATED_BODY()

public:
	FVector Position;
	int32 Level = 0;
	int32 GripIndex = 0;
	TArray<FName> Tag;
	FNeighborInfo NeighborUp;
	FNeighborInfo NeighborDown;
	TObjectPtr<FGripNode1D> PrevGrip;

public:
	FGripNode1D* GetNeighborUp(int32 Count = 1)
	{ 
		FGripNode1D* GripNode = this;
		while (GripNode && Count-- > 0)
		{
			GripNode = GripNode->NeighborUp.Neighbor;
		}
		return GripNode;
	}

	FGripNode1D* GetNeighborDown(int32 Count = 1)
	{
		FGripNode1D* GripNode = this;
		while (GripNode && Count-- > 0)
		{
			GripNode = GripNode->NeighborDown.Neighbor;
		}
		return GripNode;
	}



	inline bool operator==(const FGripNode1D& Other) const
	{
		return Position == Other.Position;
	}
};

USTRUCT(BlueprintType)
struct FLadderClimbCurveSet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enter")
		TObjectPtr<UCurveVector> BottomEnterBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enter")
		TObjectPtr<UCurveVector> TopEnterBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exit")
		TObjectPtr<UCurveVector> BottomExitBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exit")
		TObjectPtr<UCurveVector> TopExitBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbUp")
		TObjectPtr<UCurveVector> ClimbUpRightBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbUp")
		TObjectPtr<UCurveVector> ClimbUpLeftBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbUp")
		TObjectPtr<UCurveVector> ClimbUpLeftHandCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbUp")
		TObjectPtr<UCurveVector> ClimbUpRightHandCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbUp")
		TObjectPtr<UCurveVector> ClimbUpLeftFootCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbUp")
		TObjectPtr<UCurveVector> ClimbUpRightFootCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbDown")
		TObjectPtr<UCurveVector> ClimbDownRightBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbDown")
		TObjectPtr<UCurveVector> ClimbDownLeftBodyCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbDown")
		TObjectPtr<UCurveVector> ClimbDownLeftHandCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbDown")
		TObjectPtr<UCurveVector> ClimbDownRightHandCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbDown")
		TObjectPtr<UCurveVector> ClimbDownLeftFootCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ClimbDown")
		TObjectPtr<UCurveVector> ClimbDownRightFootCurve;
};

UCLASS()
class UE5PROJECT_API UClimbHeader : public UObject
{
	GENERATED_BODY()

};
