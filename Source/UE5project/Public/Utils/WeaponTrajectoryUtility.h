#pragma once

#include "CoreMinimal.h"

class USceneComponent;
class USkeletalMeshComponent;

struct FWeaponTrajectoryGeometry
{
	FTransform WeaponRelativeToBone = FTransform::Identity;
	FVector StartSocketInWeapon = FVector::ZeroVector;
	FVector EndSocketInWeapon = FVector::ZeroVector;

	bool IsValid() const { return bValid; }

private:
	friend struct FWeaponTrajectoryUtility;
	bool bValid = false;
};

/** Shared conversion from a baked bone trajectory to weapon socket positions. */
struct UE5PROJECT_API FWeaponTrajectoryUtility
{
	static FWeaponTrajectoryGeometry BuildGeometry(
		const USkeletalMeshComponent* MeshComp,
		const USceneComponent* WeaponComponent,
		FName BoneName,
		FName StartSocket,
		FName EndSocket);

	static void GetSocketWorldPositions(
		const FWeaponTrajectoryGeometry& Geometry,
		const FTransform& BoneRelativeToRoot,
		const FTransform& RootWorld,
		FVector& OutStart,
		FVector& OutEnd);
};
