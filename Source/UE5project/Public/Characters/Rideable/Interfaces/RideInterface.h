// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RideInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URideInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UE5PROJECT_API IRideInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		float GetRideSpeed();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		float GetRideDirection();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		bool GetMountDir();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		FTransform GetMountTransform();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		USceneComponent* GetLeftInteractLocation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void Mount(ACharacter* RiderCharacter, FVector InitVelocity);

	void DisMount();
};
