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
	UPROPERTY(EditAnywhere, Category = "Curve")
	TMap<FClimbCurveKey, UCurveVector*> Curves;

	UPROPERTY(EditAnywhere, Category = "Montage")
	TMap<EClimbPhase, UAnimMontage*> Montages;
};
