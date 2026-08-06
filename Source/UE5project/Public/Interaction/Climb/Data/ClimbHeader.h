// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ClimbHeader.generated.h"

UENUM(BlueprintType)
enum class EClimbPhase : uint8
{
	Enter_From_Bottom UMETA(DisplayName = "Enter_From_Bottom"),
	Enter_From_Top UMETA(DisplayName = "Enter_From_Top"),
	// Legacy values are kept hidden to preserve serialized enum indices in
	// existing animation Blueprints and data assets.
	Idle UMETA(Hidden),
	Idle_OneStep UMETA(Hidden),
	ClimbUp_Right UMETA(DisplayName = "ClimbUp_Right"),
	ClimbUp_Left UMETA(DisplayName = "ClimbUp_Left"),
	ClimbUp_OneStep UMETA(Hidden),
	ClimbDown_Right UMETA(DisplayName = "ClimbDown_Right"),
	ClimbDown_Left UMETA(DisplayName = "ClimbDown_Left"),
	ClimbDown_OneStep UMETA(Hidden),
	Exit_From_Bottom_Right UMETA(DisplayName = "Exit_From_Bottom_Right"),
	Exit_From_Bottom_Left UMETA(DisplayName = "Exit_From_Bottom_Left"),
	Exit_From_Top_Right UMETA(DisplayName = "Exit_From_Top_Right"),
	Exit_From_Top_Left UMETA(DisplayName = "Exit_From_Top_Left"),
	Idle_Right UMETA(DisplayName = "Idle_Right"),
	Idle_Left UMETA(DisplayName = "Idle_Left"),
};

UENUM(BlueprintType)
enum class ELadderActionState : uint8
{
	Detached,
	Entering,
	Idle,
	ClimbingStep,
	Recovering,
	Exiting
};

UENUM(BlueprintType)
enum class ELadderGripDirection : uint8
{
	Up,
	Down
};

struct FGripNode1D
{
	FVector LocalPosition;
	float ClimbCoordinate = 0.0f;
	int32 NeighborUpIndex = INDEX_NONE;
	int32 NeighborDownIndex = INDEX_NONE;
};
