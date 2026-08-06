// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/Climbable/Ladder/LadderBase.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Utils/CoreLog.h"

ALadderBase::ALadderBase()
{
	Tags.Add("Ladder");

	LadderGeometryRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LadderGeometryRoot"));
	LadderGeometryRoot->SetupAttachment(ObjectRoot);

	GeneratedLadderMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GeneratedLadderMesh"));
	GeneratedLadderMesh->SetupAttachment(LadderGeometryRoot);
	GeneratedLadderMesh->SetCanEverAffectNavigation(false);
}
void ALadderBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildLadder();
}

void ALadderBase::ClearGeneratedLadder()
{
	if (IsValid(GeneratedLadderMesh))
	{
		GeneratedLadderMesh->ClearInstances();
	}

	// Remove modules serialized by the old per-component construction layout.
	for (UStaticMeshComponent* ClimbMesh : ClimbMeshes)
	{
		if (IsValid(ClimbMesh))
		{
			ClimbMesh->DestroyComponent();
		}
	}
	ClimbMeshes.Empty();

	GripList1D.Empty();
}

void ALadderBase::RebuildLadder()
{
	ClearGeneratedLadder();

	if (LadderLevel <= 0 || !IsValid(ClimbStaticMesh) || !IsValid(LadderGeometryRoot) ||
		!IsValid(GeneratedLadderMesh))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[Ladder] Invalid construction settings on '%s': LadderLevel=%d, ClimbStaticMesh=%s"),
			*GetName(), LadderLevel, *GetNameSafe(ClimbStaticMesh));
		return;
	}

	LadderGeometryRoot->SetRelativeRotation(FRotator(LadderTiltAngle, 0.0f, 0.0f));
	GeneratedLadderMesh->SetStaticMesh(ClimbStaticMesh);

	const float ModuleHeight =
		ClimbStaticMesh->GetBoundingBox().GetSize().Z *
		FMath::Abs(LadderScale.Z);
	if (ModuleHeight <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(Log_Climb_Ladder, Error,
			TEXT("[Ladder] Cannot build '%s': module height is zero."),
			*GetName());
		return;
	}

	float CumulativeHeight = 0.0f;

	for (int32 i = 0; i < LadderLevel; i++)
	{
		const FTransform InstanceTransform(FRotator(0.0f, -90.0f, 0.0f),
			FVector(0.0f, 0.0f, AdditionalHeight + CumulativeHeight), LadderScale);
		GeneratedLadderMesh->AddInstance(InstanceTransform);
		CumulativeHeight += ModuleHeight;
	}

	const FVector BottomEndpoint = GetGeometryPointInActorSpace(0.0f);
	const FVector TopEndpoint = GetGeometryPointInActorSpace(AdditionalHeight + CumulativeHeight);
	ClimbBottomTrigger->SetRelativeLocation(BottomEndpoint + BottomTriggerOffset);
	ClimbBottomLocation->SetRelativeLocation(BottomEndpoint + BottomEntryOffset);
	ClimbTopTrigger->SetRelativeLocation(TopEndpoint + FVector(-80.0f, 0.0f, ClimbTopTrigger->Bounds.BoxExtent.Z));
	ClimbTopApproachLocation->SetRelativeLocation(TopEndpoint + FVector(-80.0f, 0.0f, 92.0f));
	ClimbTopLocation->SetRelativeLocation(TopEndpoint + FVector(-20.0f, 0.0f, 92.0f));
	ClimbTopExitLocation->SetRelativeLocation(TopEndpoint + FVector(-80.0f, 0.0f, 92.0f));
	// The interaction move uses this component's rotation as the montage
	// start rotation. Top entry must already face the ladder before root
	// motion and motion warping begin.
	ClimbTopLocation->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ClimbTopApproachLocation->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ClimbTopExitLocation->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}

bool ALadderBase::HasValidGeneratedMeshes() const
{
	if (LadderLevel <= 0 || !IsValid(ClimbStaticMesh) || !IsValid(GeneratedLadderMesh) ||
		GeneratedLadderMesh->GetStaticMesh() != ClimbStaticMesh || GeneratedLadderMesh->GetInstanceCount() != LadderLevel)
	{
		return false;
	}
	return true;
}

void ALadderBase::BuildRuntimeGripData()
{
	GripList1D.Empty();

	if (!HasValidGeneratedMeshes())
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[Ladder] Cannot build runtime Grip data for '%s': generated meshes are invalid."), *GetName());
		return;
	}

	const TArray<FName> SocketNames = GeneratedLadderMesh->GetAllSocketNames();
	for (int32 InstanceIndex = 0; InstanceIndex < GeneratedLadderMesh->GetInstanceCount(); ++InstanceIndex)
	{
		FTransform InstanceWorldTransform;
		if (!GeneratedLadderMesh->GetInstanceTransform(InstanceIndex, InstanceWorldTransform, true))
		{
			continue;
		}

		for (const FName SocketName : SocketNames)
		{
			if (!SocketName.ToString().Contains(TEXT("Grip")))
			{
				continue;
			}

			const UStaticMeshSocket* Socket = ClimbStaticMesh->FindSocket(SocketName);
			if (!IsValid(Socket))
			{
				continue;
			}

			const FVector SocketWorldLocation = InstanceWorldTransform.TransformPosition(Socket->RelativeLocation);
			const FVector LocalSocketLocation = GetActorTransform().InverseTransformPosition(SocketWorldLocation);
			FGripNode1D GripNode;
			GripNode.LocalPosition = LocalSocketLocation;
			GripNode.ClimbCoordinate = GeneratedLadderMesh->GetComponentTransform()
				.InverseTransformPosition(SocketWorldLocation).Z;
			GripList1D.Add(GripNode);
		}
	}

	SetInitTopPosition();
	SetInitBottomPosition();

	if (GripList1D.IsEmpty())
	{
		UE_LOG(Log_Climb_Ladder, Error,
			TEXT("[Ladder] No Grip sockets were found on the generated meshes for '%s'."),
			*GetName());
	}
}

FVector ALadderBase::GetGeometryPointInActorSpace(float Height) const
{
	return IsValid(LadderGeometryRoot)
		? LadderGeometryRoot->GetRelativeTransform().TransformPosition(FVector(0.0f, 0.0f, Height))
		: FVector(0.0f, 0.0f, Height);
}

FVector ALadderBase::GetLadderForwardVector() const
{
	return IsValid(LadderGeometryRoot) ? LadderGeometryRoot->GetForwardVector() : GetActorForwardVector();
}

FVector ALadderBase::GetLadderRightVector() const
{
	return IsValid(LadderGeometryRoot) ? LadderGeometryRoot->GetRightVector() : GetActorRightVector();
}

FVector ALadderBase::GetLadderUpVector() const
{
	return IsValid(LadderGeometryRoot) ? LadderGeometryRoot->GetUpVector() : GetActorUpVector();
}

FVector ALadderBase::GetLadderTopEndpointWorld() const
{
	if (!IsValid(ClimbStaticMesh) || LadderLevel <= 0)
	{
		return GetActorLocation();
	}

	const float ModuleHeight = ClimbStaticMesh->GetBoundingBox().GetSize().Z * FMath::Abs(LadderScale.Z);
	const FVector ActorLocalEndpoint = GetGeometryPointInActorSpace(AdditionalHeight + ModuleHeight * LadderLevel);
	return GetActorTransform().TransformPosition(ActorLocalEndpoint);
}

void ALadderBase::SetInitTopPosition()
{
	float TraceDistance = 300.0f;
	FVector StartLoc = ClimbTopLocation->GetComponentLocation();
	FVector EndLoc = StartLoc - FVector(0.0f, 0.0f, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		StartLoc,
		EndLoc,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(20.0f),
		CollisionParams
	);

	if (bHit)
	{
		ClimbTopLocation->SetWorldLocation(HitResult.ImpactPoint);
		FVector ApproachLocation =
			ClimbTopApproachLocation->GetComponentLocation();
		ApproachLocation.Z = HitResult.ImpactPoint.Z;
		ClimbTopApproachLocation->SetWorldLocation(ApproachLocation);
		FVector ExitLocation =
			ClimbTopExitLocation->GetComponentLocation();
		ExitLocation.Z = HitResult.ImpactPoint.Z;
		ClimbTopExitLocation->SetWorldLocation(ExitLocation);
	}
	else
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[Ladder] Top ground trace did not hit for '%s'."), *GetName());
	}
}

void ALadderBase::SetInitBottomPosition()
{
	float TraceDistance = 300.0f;
	FVector StartLoc = ClimbBottomLocation->GetComponentLocation();
	StartLoc.Z += 200.0f;
	FVector EndLoc = StartLoc - FVector(0.0f, 0.0f, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		StartLoc,
		EndLoc,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(20.0f),
		CollisionParams
	);

	if (bHit)
	{
		ClimbBottomLocation->SetWorldLocation(HitResult.ImpactPoint);
		FVector TriggerLocation = ClimbBottomTrigger->GetComponentLocation();
		TriggerLocation.Z = HitResult.ImpactPoint.Z + BottomTriggerOffset.Z;
		ClimbBottomTrigger->SetWorldLocation(TriggerLocation);
	}
	else
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[Ladder] Bottom ground trace did not hit for '%s'."), *GetName());
	}
}

void ALadderBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasValidGeneratedMeshes())
	{
		UE_LOG(Log_Climb_Ladder, Warning, TEXT("[Ladder] Rebuilding invalid generated meshes once at runtime for '%s'."), *GetName());
		RebuildLadder();
	}

	BuildRuntimeGripData();
}
