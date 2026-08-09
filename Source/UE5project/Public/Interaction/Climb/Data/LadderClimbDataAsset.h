// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Data/IKData.h"
#include "Engine/DataAsset.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "LadderClimbDataAsset.generated.h"

class UAnimMontage;
class UAnimSequence;
class UCurveVector;

UENUM(BlueprintType)
enum class ELadderWarpTargetAnchor : uint8
{
	TransitionTarget,
	LadderTopEndpoint
};

USTRUCT(BlueprintType)
struct FLadderWarpTargetDefinition
{
	GENERATED_BODY()

	// Must match the target name on the montage's Motion Warping notify state.
	UPROPERTY(EditAnywhere, Category = "Warp Target")
	FName TargetName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Warp Target")
	ELadderWarpTargetAnchor Anchor = ELadderWarpTargetAnchor::TransitionTarget;

	// Offset from the selected anchor in ladder-local Forward/Right/Up axes.
	UPROPERTY(EditAnywhere, Category = "Warp Target", meta = (Units = "cm"))
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Warp Target")
	FRotator RotationOffset = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FLadderPhaseWarpTargets
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Warp Target")
	TArray<FLadderWarpTargetDefinition> Targets;
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
	TObjectPtr<UAnimSequence> Animation = nullptr;

	UPROPERTY(EditAnywhere, Category = "Repeated Step", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;

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
	UPROPERTY(EditAnywhere, Category = "Idle")
	TObjectPtr<UAnimSequence> IdleRightAnimation = nullptr;

	UPROPERTY(EditAnywhere, Category = "Idle")
	TObjectPtr<UAnimSequence> IdleLeftAnimation = nullptr;

	UPROPERTY(EditAnywhere, Category = "Repeated Step")
	TMap<EClimbPhase, FLadderRepeatedStepDefinition> RepeatedSteps;

	// Time reserved for the Step -> Idle blend before a held input may start
	// the next discrete step. Keep this equal to the AnimBP transition duration.
	UPROPERTY(EditAnywhere, Category = "Repeated Step", meta = (ClampMin = "0.0", Units = "s"))
	float RepeatedStepRecoveryDuration = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Montage")
	TMap<EClimbPhase, UAnimMontage*> Montages;

	UPROPERTY(EditAnywhere, Category = "Montage")
	TMap<EClimbPhase, FLadderPhaseWarpTargets> WarpTargetsByPhase;

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

	// Error at or below this value is considered a natural fit. Candidates are
	// still allowed up to MaximumGripPatternError so different rung intervals
	// can resolve to different step counts without requiring another profile.
	UPROPERTY(EditAnywhere, Category = "Grip", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredGripPatternError = 10.0f;

	// Absolute per-limb error limit used when fitting authored body-space
	// offsets to the ladder's discrete grips.
	UPROPERTY(EditAnywhere, Category = "Grip", meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumGripPatternError = 20.0f;

	// Offsets from the centroid of all four final limb grips. These values are
	// authored for the character body type and ladder animation set.
	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	float BodyAnchorForwardOffset = 55.0f;

	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	float BodyAnchorRightOffset = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	float BodyAnchorUpOffset = 3.0f;
};
