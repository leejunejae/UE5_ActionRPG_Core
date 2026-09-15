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

struct FWeaponTraceBox
{
	FVector Center = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	FVector HalfExtent = FVector::ZeroVector;
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

	static bool BuildBox(
		const FBoneTransformSegment& Segment,
		const FWeaponTrajectoryGeometry& Geometry,
		const FTransform& PreviousRootWorld,
		const FTransform& CurrentRootWorld,
		float SampleTime,
		float SampleAlpha,
		const FVector& HalfExtent,
		FWeaponTraceBox& OutBox);
};
