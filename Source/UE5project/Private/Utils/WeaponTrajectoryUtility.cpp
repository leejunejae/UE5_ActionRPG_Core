#include "Utils/WeaponTrajectoryUtility.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"

FWeaponTrajectoryGeometry FWeaponTrajectoryUtility::BuildGeometry(
	const USkeletalMeshComponent* MeshComp,
	const USceneComponent* WeaponComponent,
	FName BoneName,
	FName StartSocket,
	FName EndSocket)
{
	FWeaponTrajectoryGeometry Result;
	if (!MeshComp || !WeaponComponent || BoneName.IsNone())
	{
		return Result;
	}

	// Use transforms evaluated by the same skeletal mesh. During an anim notify the
	// attached weapon component can still contain the previous frame's world transform;
	// comparing that transform with the current bone creates a frame-rate-dependent
	// offset. Reconstruct the attachment chain in component space instead.
	if (WeaponComponent->GetAttachParent() == MeshComp)
	{
		const FTransform BoneInMesh = MeshComp->GetSocketTransform(BoneName, RTS_Component);
		const FName AttachSocket = WeaponComponent->GetAttachSocketName();
		const FTransform AttachSocketInMesh = AttachSocket.IsNone()
			? FTransform::Identity
			: MeshComp->GetSocketTransform(AttachSocket, RTS_Component);
		const FTransform AttachSocketRelativeToBone =
			AttachSocketInMesh.GetRelativeTransform(BoneInMesh);
		Result.WeaponRelativeToBone =
			WeaponComponent->GetRelativeTransform() * AttachSocketRelativeToBone;
	}
	else
	{
		const FTransform CurrentBoneWorld = MeshComp->GetSocketTransform(BoneName, RTS_World);
		Result.WeaponRelativeToBone =
			WeaponComponent->GetComponentTransform().GetRelativeTransform(CurrentBoneWorld);
	}
	Result.StartSocketInWeapon =
		WeaponComponent->GetSocketTransform(StartSocket, RTS_Component).GetLocation();
	Result.EndSocketInWeapon =
		WeaponComponent->GetSocketTransform(EndSocket, RTS_Component).GetLocation();
	Result.bValid = true;
	return Result;
}

void FWeaponTrajectoryUtility::GetSocketWorldPositions(
	const FWeaponTrajectoryGeometry& Geometry,
	const FTransform& BoneRelativeToRoot,
	const FTransform& RootWorld,
	FVector& OutStart,
	FVector& OutEnd)
{
	const FTransform BoneWorld = BoneRelativeToRoot * RootWorld;
	const FTransform WeaponWorld = Geometry.WeaponRelativeToBone * BoneWorld;
	OutStart = WeaponWorld.TransformPosition(Geometry.StartSocketInWeapon);
	OutEnd = WeaponWorld.TransformPosition(Geometry.EndSocketInWeapon);
}
