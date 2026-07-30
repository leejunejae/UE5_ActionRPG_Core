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
	// Legacy values are kept hidden to preserve serialized enum indices in
	// existing animation Blueprints and data assets.
	Idle UMETA(Hidden),
	Idle_OneStep UMETA(Hidden),
	ClimbUp_Right UMETA(DisplayName = "ClimbUp_Right"),
	ClimbUp_Left UMETA(DisplayName = "ClimbUp_Left"),
	ClimbUp_OneStep UMETA(Hidden),
	ClimbDown_Right UMETA(DisplayName = "ClimbDown_Right"),
	ClimbDown_Left UMETA(DisplayName = "ClimbDown_Left"),
	ClimbDown_OneStep UMETA(Hidden),
	Exit_From_Bottom_Right UMETA(DisplayName = "Exit_From_Bottom_Right"),
	Exit_From_Bottom_Left UMETA(DisplayName = "Exit_From_Bottom_Left"),
	Exit_From_Top_Right UMETA(DisplayName = "Exit_From_Top_Right"),
	Exit_From_Top_Left UMETA(DisplayName = "Exit_From_Top_Left"),
	Idle_Right UMETA(DisplayName = "Idle_Right"),
	Idle_Left UMETA(DisplayName = "Idle_Left"),
};

UENUM(BlueprintType)
enum class ELadderTransitionState : uint8
{
	None,
	EnterBottom,
	EnterTop,
	ExitBottom,
	ExitTop
};

UENUM(BlueprintType)
enum class ELadderGripDirection : uint8
{
	Up,
	Down
};

USTRUCT(BlueprintType)
struct FClimbCurveKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
		EClimbPhase Phase = EClimbPhase::Idle_Right;

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

USTRUCT(Atomic, BlueprintType)
struct FNeighborInfo
{
	GENERATED_BODY()

	int32 NeighborIndex = INDEX_NONE;
	float Distance = 0.0f;

	inline bool operator==(const FNeighborInfo& Other) const
	{
		return NeighborIndex == Other.NeighborIndex;
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
	FVector LocalPosition;
	int32 Level = 0;
	int32 GripIndex = 0;
	TArray<FName> Tag;
	FNeighborInfo NeighborUp;
	FNeighborInfo NeighborDown;

	inline bool operator==(const FGripNode1D& Other) const
	{
		return LocalPosition == Other.LocalPosition;
	}
};

USTRUCT(BlueprintType)
struct FLadderClimbCurveSet
{
	GENERATED_BODY()
public:
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
