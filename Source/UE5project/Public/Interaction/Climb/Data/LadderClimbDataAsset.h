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

	UPROPERTY(EditAnywhere, Category = "Montage")
	FName BottomEnterWarpTargetName = TEXT("LadderAttach");

	// Final capsule/body offset from the ladder's bottom attach base.
	// X: ladder Forward, Y: ladder Right, Z: ladder Up.
	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	FVector BottomEnterBodyOffset = FVector(55.0f, 0.0f, 90.0f);

	// Limb assignment for the first four grips, ordered from bottom to top.
	// This belongs to the entry animation profile rather than the ladder actor.
	UPROPERTY(EditAnywhere, Category = "Grip")
	TArray<ELimbList> BottomEnterGripOrder =
	{
		ELimbList::FootR,
		ELimbList::FootL,
		ELimbList::HandL,
		ELimbList::HandR
	};

	// Capsule/body anchor offset from the midpoint of the supporting foot and
	// hand, measured along the ladder's local Up axis. This is authored for
	// the character body type and ladder locomotion animation set.
	UPROPERTY(EditAnywhere, Category = "Body Anchor", meta = (Units = "cm"))
	float BodyAnchorUpOffset = 3.0f;
};
