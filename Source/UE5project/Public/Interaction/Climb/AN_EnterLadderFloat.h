// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_EnterLadderFloat.generated.h"

/**
 * Deprecated compatibility shell for animation assets that still serialize
 * this notify. Ladder entry setup is now owned by UClimbComponent.
 */
UCLASS()
class UE5PROJECT_API UAN_EnterLadderFloat : public UAnimNotify
{
	GENERATED_BODY()
};
