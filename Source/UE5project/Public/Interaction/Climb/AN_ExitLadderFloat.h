// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ExitLadderFloat.generated.h"

/**
 * Deprecated compatibility shell for animation assets that still serialize
 * this notify. Montage completion now owns ladder exit cleanup.
 */
UCLASS()
class UE5PROJECT_API UAN_ExitLadderFloat : public UAnimNotify
{
	GENERATED_BODY()
};
