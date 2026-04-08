// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MovementTypes.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EDirection4Way : uint8
{
    None UMETA(DisplayName = "None"),
    Forward UMETA(DisplayName = "Forward"),
    Backward UMETA(DisplayName = "Backward"),
    Right UMETA(DisplayName = "Right"),
    Left UMETA(DisplayName = "Left")
};

UENUM(BlueprintType)
enum class EDirection8Way : uint8
{
    None UMETA(DisplayName = "None"),
    Forward UMETA(DisplayName = "Forward"),
    Backward UMETA(DisplayName = "Backward"),
    Right UMETA(DisplayName = "Right"),
    Left UMETA(DisplayName = "Left"),
    ForwardRight UMETA(DisplayName = "ForwardRight"),
    BackwardRight UMETA(DisplayName = "BackwardRight"),
    ForwardLeft UMETA(DisplayName = "ForwardLeft"),
    BackwardLeft UMETA(DisplayName = "BackWardLeft")
};

UENUM(BlueprintType)
enum class EDirection10Way : uint8
{
    None UMETA(DisplayName = "None"),
    Forward UMETA(DisplayName = "Forward"),
    Backward UMETA(DisplayName = "Backward"),
    FrontRight UMETA(DisplayName = "FrontRight"),
    FrontLeft UMETA(DisplayName = "FrontLeft"),
    BackRight UMETA(DisplayName = "BackRight"),
    BackLeft UMETA(DisplayName = "BackLeft"),
    ForwardRight UMETA(DisplayName = "ForwardRight"),
    BackwardRight UMETA(DisplayName = "BackwardRight"),
    ForwardLeft UMETA(DisplayName = "ForwardLeft"),
    BackwardLeft UMETA(DisplayName = "BackWardLeft")
};

UENUM(BlueprintType)
enum class EDirectionType : uint8
{
    FourCardinal UMETA(DisplayName = "4Way"),
    EightCardinal UMETA(DisplayName = "8Way"),
    TenCardinal UMETA(DisplayName = "10Way")
};

UENUM(BlueprintType)
enum class ELocomotionGait : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	WalkSlow UMETA(DisplayName = "WalkSlow"),
	Walk UMETA(DisplayName = "Walk"),
	Jog UMETA(DisplayName = "Jog"),
	Run UMETA(DisplayName = "Run"),
	Sprint UMETA(DisplayName = "Sprint"),
};

USTRUCT(BlueprintType)
struct FGaitSetting
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        bool bEnabled = true;           // NPC Gait 사용 여부

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float MaxSpeed = 300.f;         // CharacterMovement->MaxWalkSpeed

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float Accel = 2048.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float Decel = 2048.f;

    // 애님 재생 속도
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        float AnimPlayRate = 1.0f;
};

UCLASS()
class UE5PROJECT_API UMovementTypes : public UObject
{
	GENERATED_BODY()
	
};
