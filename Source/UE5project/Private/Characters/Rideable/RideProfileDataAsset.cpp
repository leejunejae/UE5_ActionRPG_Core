#include "Characters/Rideable/RideProfileDataAsset.h"

#include "Animation/AnimMontage.h"
#include "Curves/CurveFloat.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult URideProfileDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto Invalidate = [&Context, &Result](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};
	auto ValidateTransitionCurve = [&Invalidate](const UCurveFloat* Curve, const TCHAR* CurveLabel)
	{
		if (!Curve)
		{
			return;
		}

		float MinTime = 0.0f;
		float MaxTime = 0.0f;
		Curve->GetTimeRange(MinTime, MaxTime);
		if (MinTime > KINDA_SMALL_NUMBER || MaxTime < 1.0f - KINDA_SMALL_NUMBER)
		{
			Invalidate(FText::Format(
				NSLOCTEXT("RideProfile", "InvalidCurveTimeRange",
					"{0} must cover normalized time 0 through 1."),
				FText::FromString(FString(CurveLabel))));
		}

		if (!FMath::IsNearlyEqual(Curve->GetFloatValue(1.0f), 1.0f, 0.01f))
		{
			Invalidate(FText::Format(
				NSLOCTEXT("RideProfile", "InvalidCurveEndValue",
					"{0} must evaluate to 1 at normalized time 1."),
				FText::FromString(FString(CurveLabel))));
		}
	};

	if (!MountMontage)
	{
		Invalidate(NSLOCTEXT("RideProfile", "MissingMount", "A mount montage is required."));
	}
	if (!DismountMontage)
	{
		Invalidate(NSLOCTEXT("RideProfile", "MissingDismount", "A dismount montage is required."));
	}
	if (!MountHorizontalCurve || !MountVerticalCurve)
	{
		Invalidate(NSLOCTEXT("RideProfile", "MissingMountCurves",
			"Mount horizontal and vertical movement curves are required."));
	}
	if (!DismountHorizontalCurve || !DismountVerticalCurve)
	{
		Invalidate(NSLOCTEXT("RideProfile", "MissingDismountCurves",
			"Dismount horizontal and vertical movement curves are required."));
	}
	if (!(WalkSpeed <= RunSpeed && RunSpeed <= SprintSpeed))
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidSpeeds", "Ride speeds must satisfy Walk <= Run <= Sprint."));
	}
	if (WalkSpeed < 0.0f || RunSpeed < 0.0f || SprintSpeed < 0.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "NegativeSpeed", "Ride speeds cannot be negative."));
	}
	if (TransitionTimeout <= 0.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidTransitionTimeout", "Transition timeout must be positive."));
	}
	if (CameraBlendDuration < 0.0f || MovingDismountSpeedThreshold < 0.0f ||
		MovingDismountVelocityScale < 0.0f || DismountGroundClearance < 0.0f ||
		DismountOverlapInflationTolerance < 0.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidNonNegativeSettings",
			"Camera, dismount threshold, velocity scale, clearance, and overlap tolerance values cannot be negative."));
	}
	if (WalkInputThreshold < 0.0f || WalkInputThreshold > 1.0f ||
		InputDeadZone < 0.0f || InputDeadZone > 1.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidInputThresholds",
			"Walk input threshold and input dead zone must be between 0 and 1."));
	}
	if (MaxTurnRate < 0.0f || MinTurnRateAtMaxSpeed < 0.0f || TurnRateInterpSpeed < 0.0f ||
		VelocityHeadingInterpSpeed < 0.0f || AnimationSpeedInterpRate < 0.0f || AnimationTurnRateInterpRate < 0.0f ||
		AnimationDirectionInterpRate < 0.0f || BlendSpaceDirectionLimit < 0.0f ||
		FullTurnAuthoritySpeed < 0.0f || MinMovingTurnAuthority < 0.0f || MinMovingTurnAuthority > 1.0f ||
		MaxAnimDirection < 0.0f || MaxAnimDirection > 180.0f ||
		PivotTurnMaxStartSpeed < 0.0f || PivotTurnMinAngle < 0.0f || PivotTurnMinAngle > 180.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidTurningSettings", "Ride turning settings are outside their valid ranges."));
	}
	if (SprintForwardAlignmentThreshold < -1.0f || SprintForwardAlignmentThreshold > 1.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidSprintAlignment",
			"Sprint forward alignment threshold must be between -1 and 1."));
	}
	if (DismountGroundTraceUpDistance + DismountGroundTraceDownDistance <= 0.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidDismountGroundTrace",
			"The dismount ground sweep must cover a positive distance."));
	}
	if (PivotTurnMontage)
	{
		if (PivotTurnMontage->GetSectionIndex(PivotTurnLeftSection) == INDEX_NONE ||
			PivotTurnMontage->GetSectionIndex(PivotTurnRightSection) == INDEX_NONE)
		{
			Invalidate(NSLOCTEXT("RideProfile", "MissingPivotSections",
				"The configured pivot-turn montage must contain both configured pivot sections."));
		}
	}
	if (HandLeftSocket.IsNone() || HandRightSocket.IsNone() || FootLeftSocket.IsNone() || FootRightSocket.IsNone())
	{
		Invalidate(NSLOCTEXT("RideProfile", "MissingRideIKSocketNames", "Ride locomotion IK socket names cannot be None."));
	}

	ValidateTransitionCurve(MountHorizontalCurve, TEXT("MountHorizontalCurve"));
	ValidateTransitionCurve(MountVerticalCurve, TEXT("MountVerticalCurve"));
	ValidateTransitionCurve(DismountHorizontalCurve, TEXT("DismountHorizontalCurve"));
	ValidateTransitionCurve(DismountVerticalCurve, TEXT("DismountVerticalCurve"));
	return Result;
}
#endif
