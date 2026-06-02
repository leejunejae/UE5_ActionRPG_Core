// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/MovementTypes.h"
#include "UObject/NoExportTypes.h"
#include "StatusData.generated.h"

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
