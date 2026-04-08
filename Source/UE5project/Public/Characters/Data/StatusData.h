// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/MovementTypes.h"
#include "UObject/NoExportTypes.h"
#include "StatusData.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Ground UMETA(DisplayName = "Ground"),
	Ladder UMETA(DisplayName = "Ladder"),
	Ride UMETA(DisplayName = "Ride"),
};

UENUM(BlueprintType)
enum class EGroundStance : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Attack UMETA(DisplayName = "Attack"),
	Block UMETA(DisplayName = "Block"),
	Dodge UMETA(DisplayName = "Dodge"),
	Jump UMETA(DisplayName = "Jump"),
	Hit UMETA(DisplayName = "Hit"),
	Invincible UMETA(DisplayName = "Invincible"),
	Parry UMETA(DislplayName = "Parry"),
	SuperArmor UMETA(DisplayName = "SuperArmor"),
};

UENUM(BlueprintType)
enum class ERideStance : uint8
{
	Mount UMETA(DisplayName = "Mount"),
	DisMount UMETA(DisplayName = "DisMount"),
	Riding UMETA(DisplayName = "Riding"),
};

UCLASS()
class UE5PROJECT_API UStatusData : public UObject
{
	GENERATED_BODY()
	
};
