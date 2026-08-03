// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "LadderClimbDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FLadderWarpCheckpoint
{
	GENERATED_BODY()

	// Transition montage that consumes this checkpoint.
	UPROPERTY(EditAnywhere, Category = "Warp Checkpoint")
	EClimbPhase Phase = EClimbPhase::Enter_From_Top;

	// Must match the Motion Warping notify state's target name.
	UPROPERTY(EditAnywhere, Category = "Warp Checkpoint")
	FName TargetName = NAME_None;

	// Animation-authored checkpoint transform relative to the final body
	// target, expressed in ladder-local Forward/Right/Up axes.
	UPROPERTY(EditAnywhere, Category = "Warp Checkpoint", meta = (Units = "cm"))
	FVector OffsetFromFinalBody = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Warp Checkpoint")
	FRotator RotationOffset = FRotator::ZeroRotator;
};

UENUM(BlueprintType)
enum class ELadderSide : uint8
{
	Left,
	Right
};

USTRUCT(BlueprintType)
struct FLadderRepeatedStepDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Repeated Step")
	ELadderSide MovingHand = ELadderSide::Left;

	UPROPERTY(EditAnywhere, Category = "Repeated Step")
	ELadderSide MovingFoot = ELadderSide::Right;

	UPROPERTY(EditAnywhere, Category = "Repeated Step")
	TObjectPtr<UCurveVector> BodyCurve = nullptr;

	UPROPERTY(EditAnywhere, Category = "Repeated Step")
	TObjectPtr<UCurveVector> HandCurve = nullptr;

	UPROPERTY(EditAnywhere, Category = "Repeated Step")
	TObjectPtr<UCurveVector> FootCurve = nullptr;
};

/**
 * 
 */
UCLASS()
class UE5PROJECT_API ULadderClimbDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Curve")
	TMap<FClimbCurveKey, UCurveVector*> Curves;

	// Data-driven definitions for repeated ladder steps. When a phase exists
	// here, it takes precedence over the legacy Curves map above.
	UPROPERTY(EditAnywhere, Category = "Repeated Step")
	TMap<EClimbPhase, FLadderRepeatedStepDefinition> RepeatedSteps;

	UPROPERTY(EditAnywhere, Category = "Montage")
	TMap<EClimbPhase, UAnimMontage*> Montages;

	UPROPERTY(EditAnywhere, Category = "Montage")
	FName EnterWarpTargetName = TEXT("LadderAttach");

	// Optional animation-specific intermediate targets. An empty array keeps
	// the entry montage on the single final LadderAttach target.
	UPROPERTY(EditAnywhere, Category = "Montage")
	TArray<FLadderWarpCheckpoint> EnterWarpCheckpoints;

	UPROPERTY(EditAnywhere, Category = "Montage")
	FName ExitWarpTargetName = TEXT("LadderExit");

	UPROPERTY(EditAnywhere, Category = "Montage")
	TArray<FLadderWarpCheckpoint> ExitWarpCheckpoints;

	// A larger remaining error means motion warping did not reach the authored
	// final attach transform and must not be hidden by an end-of-montage snap.
	UPROPERTY(EditAnywhere, Category = "Montage", meta = (ClampMin = "0.0", Units = "cm"))
	float TopEnterCompletionTolerance = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Montage", meta = (ClampMin = "0.0", Units = "cm"))
	float ExitCompletionTolerance = 5.0f;

	// Maximum body-position error accepted when one repeated climb step ends.
	// Errors inside this range are finalized through CharacterMovement;
	// larger errors indicate a broken curve or blocked movement.
	UPROPERTY(EditAnywhere, Category = "Curve", meta = (ClampMin = "0.0", Units = "cm"))
	float RepeatedClimbCompletionTolerance = 3.0f;

	// Final idle pose reached by the bottom-entry animation. Values are
	// measured from BottomEnterIdleReferenceLimb along ladder-local Up.
	UPROPERTY(EditAnywhere, Category = "Grip|Bottom Enter|Final Idle")
	TMap<ELimbList, float> BottomEnterIdleGripHeightOffsets =
	{
		{ ELimbList::HandR, 0.0f },
		{ ELimbList::HandL, -30.0f },
		{ ELimbList::FootL, -90.0f },
		{ ELimbList::FootR, -120.0f }
	};

	UPROPERTY(EditAnywhere, Category = "Grip|Bottom Enter|Final Idle")
	ELimbList BottomEnterIdleReferenceLimb = ELimbList::HandR;

	// Final idle pose reached by the top-entry animation. It is intentionally
	// separate because top and bottom entry may finish on opposite step sides.
	UPROPERTY(EditAnywhere, Category = "Grip|Top Enter|Final Idle")
	TMap<ELimbList, float> TopEnterIdleGripHeightOffsets =
	{
		{ ELimbList::HandL, 0.0f },
		{ ELimbList::HandR, -30.0f },
		{ ELimbList::FootR, -90.0f },
		{ ELimbList::FootL, -120.0f }
	};

	UPROPERTY(EditAnywhere, Category = "Grip|Top Enter|Final Idle")
	ELimbList TopEnterIdleReferenceLimb = ELimbList::HandL;

	// Initial contacts authored by the top-entry animation. The route between
	// these contacts and TopEnterIdleGripHeightOffsets is distributed across
	// the montage's Ladder Grip Transition notify states.
	UPROPERTY(EditAnywhere, Category = "Grip|Top Enter|Initial")
	TMap<ELimbList, float> TopEnterInitialGripHeightOffsets =
	{
		{ ELimbList::HandL, 0.0f },
		{ ELimbList::HandR, -30.0f },
		{ ELimbList::FootL, -30.0f },
		{ ELimbList::FootR, -60.0f }
	};

	UPROPERTY(EditAnywhere, Category = "Grip|Top Enter|Initial")
	ELimbList TopEnterInitialReferenceLimb = ELimbList::HandL;

	UPROPERTY(EditAnywhere, Category = "Grip", meta = (ClampMin = "0.0", Units = "cm"))
	float GripMatchTolerance = 10.0f;

	// Offsets from the centroid of all four final limb grips. These values are
	// authored for the character body type and ladder animation set.
	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	float BodyAnchorForwardOffset = 55.0f;

	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	float BodyAnchorRightOffset = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	float BodyAnchorUpOffset = 3.0f;
};
