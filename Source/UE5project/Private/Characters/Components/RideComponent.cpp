// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Components/RideComponent.h"

#include "Characters/Rideable/Ride.h"
#include "Components/SkeletalMeshComponent.h"

URideComponent::URideComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URideComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URideComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void URideComponent::SetCurrentRide(ARide* NewRide)
{
	CurrentRide = NewRide;
}

void URideComponent::ClearCurrentRide()
{
	CurrentRide = nullptr;
}

float URideComponent::GetRideSpeed() const
{
	return CurrentRide ? CurrentRide->GetRideSpeed() : 0.0f;
}

float URideComponent::GetRideDirection() const
{
	return CurrentRide ? CurrentRide->GetRideDirection() : 0.0f;
}

FTransform URideComponent::GetMountTransform() const
{
	return CurrentRide ? CurrentRide->GetMountTransform() : FTransform::Identity;
}

FTransform URideComponent::GetDisMountTransform() const
{
	return  CurrentRide ? CurrentRide->GetDismountTransform() : FTransform::Identity;
}

TOptional<FVector> URideComponent::GetRideIKTargetLoc(EBodyType BoneType) const
{
	if (!CurrentRide || !CurrentRide->GetMesh())
		return TOptional<FVector>();

	USkeletalMeshComponent* RideMesh = CurrentRide->GetMesh();
	FName SocketName;

	switch (BoneType)
	{
	case EBodyType::Hand_L:
		SocketName = FName("Reins_Bn_Hand_L");
		break;
	case EBodyType::Hand_R:
		SocketName = FName("Reins_Bn_Hand_R");
		break;
	case EBodyType::Foot_L:
		SocketName = FName("SaddleLeftFootPlace");
		break;
	case EBodyType::Foot_R:
		SocketName = FName("SaddleRightFootPlace");
		break;
	default:
		return TOptional<FVector>();
	}

	return RideMesh->DoesSocketExist(SocketName)
		? TOptional<FVector>(RideMesh->GetSocketLocation(SocketName))
		: TOptional<FVector>();
}

