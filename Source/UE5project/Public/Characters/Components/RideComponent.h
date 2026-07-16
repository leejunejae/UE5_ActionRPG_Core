// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/Data/BaseCharacterHeader.h"
#include "Characters/Data/StatusData.h"
#include "RideComponent.generated.h"

class ARide;
class APlayerBase;
class APlayerRide;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE5PROJECT_API URideComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URideComponent();

protected:
	virtual void BeginPlay() override;

public:	
	void SetCurrentRide(ARide* NewRide);
	void ClearCurrentRide();

	ARide* GetCurrentRide() const { return CurrentRide; }
	ERideAnimPhase GetCurRideAnimPhase() const { return CurRideAnimPhase; }

	void RequestSpawnRide();
	bool RequestDismount(FVector InitVelocity);
	void HandleMountEnd();
	void HandleDismountEnd();

	float GetRideSpeed() const;
	float GetRideDirection() const;
	FTransform GetMountTransform() const;
	FTransform GetDisMountTransform() const;
	TOptional<FVector> GetRideIKTargetLoc(EBodyType BoneType) const;

private:
	void BeginRideCollision();
	void EndRideCollision(ARide* Ride);
	void MountTimer();
	void NormalDismountTimer();
	void BlendPlayerCameraToRide(ARide* Ride, FVector InitVelocity);
	void BlendRideCameraToPlayer();

	UPROPERTY(Transient)
	TObjectPtr<APlayerBase> Player = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ARide> CurrentRide = nullptr;

	ERideAnimPhase CurRideAnimPhase = ERideAnimPhase::Riding;

	FTimerHandle MountTimerHandle;
	FTimerHandle NormalDismountTimerHandle;
	FTransform NormalDismountStartTransform;
	FTransform NormalDismountTargetTransform;
};
