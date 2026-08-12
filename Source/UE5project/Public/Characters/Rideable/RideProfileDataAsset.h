#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RideProfileDataAsset.generated.h"

class UAnimMontage;
class UCurveFloat;

UCLASS(BlueprintType)
class UE5PROJECT_API URideProfileDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Transition")
	TObjectPtr<UAnimMontage> MountMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Transition")
	TObjectPtr<UAnimMontage> DismountMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Transition|Movement Curves")
	TObjectPtr<UCurveFloat> MountHorizontalCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Transition|Movement Curves")
	TObjectPtr<UCurveFloat> MountVerticalCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Transition|Movement Curves")
	TObjectPtr<UCurveFloat> DismountHorizontalCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Transition|Movement Curves")
	TObjectPtr<UCurveFloat> DismountVerticalCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0.1", Units = "s"))
	float TransitionTimeout = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0.0", Units = "s"))
	float CameraBlendDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismount", meta = (ClampMin = "0.0"))
	float MovingDismountSpeedThreshold = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismount", meta = (ClampMin = "0.0"))
	float MovingDismountVelocityScale = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismount")
	float MovingDismountVerticalVelocity = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismount|Grounding", meta = (ClampMin = "0.0", Units = "cm"))
	float DismountGroundTraceUpDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismount|Grounding", meta = (ClampMin = "0.0", Units = "cm"))
	float DismountGroundTraceDownDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismount|Grounding", meta = (ClampMin = "0.0", Units = "cm"))
	float DismountGroundClearance = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismount|Grounding", meta = (ClampMin = "0.0", Units = "cm"))
	float DismountOverlapInflationTolerance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float RunSpeed = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float SprintSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkInputThreshold = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float AccelerationInterpSpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float DecelerationInterpSpeed = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "deg/s"))
	float MaxTurnRate = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "deg/s"))
	float MinTurnRateAtMaxSpeed = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InputDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "deg/s"))
	float DirectionInterpRate = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "deg"))
	float MaxAnimDirection = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot Turn", meta = (ClampMin = "0.0"))
	float PivotTurnMaxStartSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot Turn", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float PivotTurnMinAngle = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot Turn")
	TObjectPtr<UAnimMontage> PivotTurnMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot Turn")
	TObjectPtr<UCurveFloat> PivotTurnAlphaCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot Turn")
	bool bUseNormalizedPivotTurnCurveTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot Turn")
	FName PivotTurnLeftSection = TEXT("TurnLeft");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot Turn")
	FName PivotTurnRightSection = TEXT("TurnRight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK")
	FName HandLeftSocket = TEXT("Hand_L_RideIK");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK")
	FName HandRightSocket = TEXT("Hand_R_RideIK");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK")
	FName FootLeftSocket = TEXT("Foot_L_RideIK");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK")
	FName FootRightSocket = TEXT("Foot_R_RideIK");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK|Transition")
	FName TransitionHandLeftSocket = TEXT("Reins_Bn_Hand_L");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK|Transition")
	FName TransitionHandRightSocket = TEXT("Reins_Bn_Hand_R");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK|Transition")
	FName TransitionFootLeftSocket = TEXT("SaddleLeftFootPlace");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK|Transition")
	FName TransitionFootRightSocket = TEXT("SaddleRightFootPlace");

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
