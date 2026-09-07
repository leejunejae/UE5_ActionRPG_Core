#pragma once

#include "CoreMinimal.h"

struct FBoneTransformSegment;
struct FWeaponTrajectoryGeometry;

struct FWeaponTraceCapsule
{
	FVector Center = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	float Radius = 0.0f;
	float HalfHeight = 0.0f;
};

/** Stateless geometry used by AttackComponent's gameplay hit processing. */
struct FWeaponTraceSolver
{
	static bool BuildCapsule(
		const FBoneTransformSegment& Segment,
		const FWeaponTrajectoryGeometry& Geometry,
		const FTransform& PreviousRootWorld,
		const FTransform& CurrentRootWorld,
		float SampleTime,
		float SampleAlpha,
		float Radius,
		FWeaponTraceCapsule& OutCapsule);
};
