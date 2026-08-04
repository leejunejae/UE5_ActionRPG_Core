// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/Climbable/Ladder/LadderBase.h"

#include "Components/BoxComponent.h"
#include "Utils/CoreLog.h"

ALadderBase::ALadderBase()
{
	Tags.Add("Ladder");
	LadderScale = FVector(1.0f, 1.0f, 1.0f);
	AdditionalHeight = 0.0f;

	ClimbObjectTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Climbable.Ladder")));

}
void ALadderBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildLadder();
}

void ALadderBase::ClearGeneratedLadder()
{
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

	if (LadderLevel <= 0 || !IsValid(ClimbStaticMesh))
	{
		UE_LOG(Log_Climb_Ladder, Error, TEXT("[Ladder] Invalid construction settings on '%s': LadderLevel=%d, ClimbStaticMesh=%s"),
			*GetName(), LadderLevel, *GetNameSafe(ClimbStaticMesh));
		return;
	}

	float CumulativeHeight = 0.0f;

	for (int32 i = 0; i < LadderLevel; i++)
	{
		UStaticMeshComponent* NewClimbMesh = NewObject<UStaticMeshComponent>(this);
		NewClimbMesh->SetStaticMesh(ClimbStaticMesh);
		NewClimbMesh->SetupAttachment(RootComponent);
		NewClimbMesh->SetCanEverAffectNavigation(false);
		NewClimbMesh->SetRelativeScale3D(LadderScale);
		NewClimbMesh->SetRelativeLocation(FVector(0.0f, 0.0f, AdditionalHeight + CumulativeHeight));
		NewClimbMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		NewClimbMesh->RegisterComponent();
		CumulativeHeight += NewClimbMesh->Bounds.BoxExtent.Z * 2.0f;
		ClimbMeshes.Add(NewClimbMesh);
	}

	ClimbTopTrigger->SetRelativeLocation(FVector(-80.0f, 0.0f, AdditionalHeight + CumulativeHeight + ClimbTopTrigger->Bounds.BoxExtent.Z));
	ClimbTopApproachLocation->SetRelativeLocation(
		FVector(-80.0f, 0.0f, AdditionalHeight + CumulativeHeight + 92.0f));
	ClimbTopLocation->SetRelativeLocation(
		FVector(-20.0f, 0.0f, AdditionalHeight + CumulativeHeight + 92.0f));
	ClimbTopExitLocation->SetRelativeLocation(
		FVector(-80.0f, 0.0f, AdditionalHeight + CumulativeHeight + 92.0f));
	// The interaction move uses this component's rotation as the montage
	// start rotation. Top entry must already face the ladder before root
	// motion and motion warping begin.
	ClimbTopLocation->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ClimbTopApproachLocation->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ClimbTopExitLocation->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}

bool ALadderBase::HasValidGeneratedMeshes() const
{
	if (LadderLevel <= 0 || !IsValid(ClimbStaticMesh) || ClimbMeshes.Num() != LadderLevel)
	{
		return false;
	}

	for (const UStaticMeshComponent* ClimbMesh : ClimbMeshes)
	{
		if (!IsValid(ClimbMesh) || ClimbMesh->GetStaticMesh() != ClimbStaticMesh)
		{
			return false;
		}
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

	for (int32 MeshIndex = 0; MeshIndex < ClimbMeshes.Num(); ++MeshIndex)
	{
		UStaticMeshComponent* ClimbMesh = ClimbMeshes[MeshIndex];
		for (const FName SocketName : ClimbMesh->GetAllSocketNames())
		{
			if (!SocketName.ToString().Contains(TEXT("Grip")))
			{
				continue;
			}

			const FVector LocalSocketLocation = GetActorTransform().InverseTransformPosition(
				ClimbMesh->GetSocketLocation(SocketName));
			GripList1D.Add({ LocalSocketLocation, MeshIndex + 1 });
		}
	}

	SetInitTopPosition();
	SetInitBottomPosition();

	GripList1D.Sort([](const FGripNode1D& A, const FGripNode1D& B)
	{
		return A.LocalPosition.Z < B.LocalPosition.Z;
	});

	for (int32 GripIndex = 0; GripIndex < GripList1D.Num(); ++GripIndex)
	{
		GripList1D[GripIndex].GripIndex = GripIndex;
	}
}

void ALadderBase::SetInitTopPosition()
{
	float TraceDistance = 300.0f;
	FVector StartLoc = ClimbTopLocation->GetComponentLocation();
	FVector EndLoc = StartLoc - FVector(0.0f, 0.0f, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());
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
	CollisionParams.AddIgnoredActor(GetOwner());
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
