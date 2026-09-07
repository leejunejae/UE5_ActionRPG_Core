#include "Combat/Trajectory/WeaponTraceSolver.h"

#include "Utils/AttackBoneDataRegistry.h"
#include "Utils/WeaponTrajectoryUtility.h"

bool FWeaponTraceSolver::BuildCapsule(
	const FBoneTransformSegment& Segment,
	const FWeaponTrajectoryGeometry& Geometry,
	const FTransform& PreviousRootWorld,
	const FTransform& CurrentRootWorld,
	float SampleTime,
	float SampleAlpha,
	float Radius,
	FWeaponTraceCapsule& OutCapsule)
{
	FTransform RootWorld;
	RootWorld.Blend(PreviousRootWorld, CurrentRootWorld, SampleAlpha);

	FVector Start;
	FVector End;
	FWeaponTrajectoryUtility::GetSocketWorldPositions(
		Geometry, Segment.GetTransformAtTime(SampleTime), RootWorld, Start, End);

	const FVector Axis = End - Start;
	if (Axis.IsNearlyZero()) return false;

	OutCapsule.Center = (Start + End) * 0.5f;
	OutCapsule.Rotation = FRotationMatrix::MakeFromZ(Axis.GetSafeNormal()).ToQuat();
	OutCapsule.Radius = Radius;
	OutCapsule.HalfHeight = FMath::Max(Axis.Size() * 0.5f, Radius);
	return true;
}
