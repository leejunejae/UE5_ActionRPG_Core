// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/Data/BaseCharacterHeader.h"
#include "RideComponent.generated.h"

class ARide;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE5PROJECT_API URideComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URideComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetCurrentRide(ARide* NewRide);
	void ClearCurrentRide();

	ARide* GetCurrentRide() const { return CurrentRide; }
	float GetRideSpeed() const;
	float GetRideDirection() const;
	FTransform GetMountTransform() const;
	FTransform GetDisMountTransform() const;
	TOptional<FVector> GetRideIKTargetLoc(EBodyType BoneType) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ARide> CurrentRide = nullptr;
};
