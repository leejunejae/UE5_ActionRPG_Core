// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LadderInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULadderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UE5PROJECT_API ILadderInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		const USceneComponent* GetInitClimbTarget() const; // 위에서 사다리에 올라탔을때 위치

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		const USceneComponent* GetInitEnterTarget(bool IsTop) const; // 위에서 사다리에 올라탈 때 시작위치, 내릴 때 마무리 위치

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		const USceneComponent* GetTopEnterHandTarget(bool IsRight) const; // 아래에서 사다리에 올라탈 때 시작위치, 내릴 때 마무리 위치
};
