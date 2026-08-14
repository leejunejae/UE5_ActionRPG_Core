// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/MovementTypes.h"
#include "UObject/NoExportTypes.h"
#include "StatusData.generated.h"

UENUM(BlueprintType)
enum class EActionExitReason : uint8
{
	Completed,
	Transition,
	Locomotion,
	Interrupted,
	Death,
	EquipmentChange
};

USTRUCT(BlueprintType)
struct FActionExitBlendSettings
{
	GENERATED_BODY()

	// 음수면 몽타주 에셋의 Blend Out 설정을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Transition = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Locomotion = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Interrupted = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Death = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float EquipmentChange = -1.0f;

	float GetOverride(EActionExitReason Reason) const
	{
		switch (Reason)
		{
		case EActionExitReason::Transition: return Transition;
		case EActionExitReason::Locomotion: return Locomotion;
		case EActionExitReason::Interrupted: return Interrupted;
		case EActionExitReason::Death: return Death;
		case EActionExitReason::EquipmentChange: return EquipmentChange;
		default: return -1.0f;
		}
	}
};

UCLASS()
class UE5PROJECT_API UStatusData : public UObject
{
	GENERATED_BODY()
	
};
