// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "LadderClimbDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API ULadderClimbDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FLadderClimbCurveSet_Map CurveSet;

		//FLadderClimbCurveSet LadderClimbCurveSet;
};
