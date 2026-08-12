#include "Characters/Rideable/RideProfileDataAsset.h"

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
	if (DismountGroundTraceUpDistance + DismountGroundTraceDownDistance <= 0.0f)
	{
		Invalidate(NSLOCTEXT("RideProfile", "InvalidDismountGroundTrace",
			"The dismount ground sweep must cover a positive distance."));
	}
	return Result;
}
#endif
