// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AnimModeBase.generated.h"

/**
 * 
 */

class UCharacterBaseAnimInstance;
class ACharacterBase;

UCLASS()
class UE5PROJECT_API UAnimModeBase : public UObject
{
	GENERATED_BODY()
	
public:
	TWeakObjectPtr<ACharacterBase> Character;
	TWeakObjectPtr<UCharacterBaseAnimInstance> AnimInst;

	virtual void OnModeEnter() {}
	virtual void OnModeExit() {}
	virtual void Tick(float DeltaSeconds) {}
};
