// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 엔진 헤더
#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// 입력
#include "InputActionValue.h"

// 구조체, 자료형


// 태그
#include "GameplayTagContainer.h"


#include "Ride.generated.h"

class UCharacterMovementComponent;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class URideProfileDataAsset;
class URideComponent;
class UCurveFloat;
class UAnimMontage;
class UAnimInstance;

UENUM(BlueprintType)
enum class HorseDirection : uint8
{
	Back UMETA(DisplayName = "Back"),
	Stop UMETA(DisplayName = "Stop"),
	Front UMETA(DisplayName = "Front"),
};

UENUM(BlueprintType)
enum class ERideGait : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run"),
	Sprint UMETA(DisplayName = "Sprint"),
};

UCLASS()
class UE5PROJECT_API ARide : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARide();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
		FGameplayTagContainer RideTags;

private:
	void InputSetting();

private:
	float Direction;
	float TurnRate = 0.0f;
	bool bBraking = false;
	ERideGait CurrentGait = ERideGait::Idle;

	ACharacter* Rider;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& value);
	void StopMove(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void StartWalk(const FInputActionValue& value);
	void StopWalk(const FInputActionValue& value);
	void StartSprint(const FInputActionValue& value);
	void StopSprint(const FInputActionValue& value);
	void UpdateRideMovement(float DeltaTime);
	void UpdatePivotTurn(float DeltaTime);
	bool CanStartPivotTurn(float DotProductDegree) const;
	float GetPivotTurnCurveAlpha(UAnimInstance* AnimInstance) const;
	void FinishPivotTurn();
	float GetRideSpeedForGait(ERideGait Gait) const;
	ERideGait GetDesiredRideGait(const FVector2D& MoveInput, float ForwardAlignment) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = Equipment)
		USkeletalMeshComponent* Saddle;
	UPROPERTY(VisibleAnywhere, Category = Equipment)
		USkeletalMeshComponent* Reins;

	UPROPERTY(VisibleAnywhere, Category = Camera)
		USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
		UCameraComponent* Camera;

	/* �� �Է� */
	UPROPERTY(EditAnywhere, Category = Input)
		UInputMappingContext* DefaultContext;

	UPROPERTY(EditAnywhere, Category = Input)
		UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = Input)
		UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = Input)
		UInputAction* DisMountAction;

	UPROPERTY(EditAnywhere, Category = Input)
		UInputAction* WalkAction;

	UPROPERTY(EditAnywhere, Category = Input)
		UInputAction* SprintAction;


	FVector2D RideMoveInput = FVector2D::ZeroVector;
	float CurrentThrottle = 0.0f;
	bool bWantsWalk = false;
	bool bWantsSprint = false;

	/* �� �Է� */

	UPROPERTY(VisibleAnywhere, Category = Interact)
		USceneComponent* RiderLocation;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		USceneComponent* RiderGetDownLoc;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		USceneComponent* RiderMountLocLeft;

	UPROPERTY(VisibleAnywhere, Category = Interact)
		USceneComponent* RiderMountLocRight;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PostInitializeComponents() override;

	float GetDirection() const;
	float GetTurnRate() const { return TurnRate; }
	float GetAnimationSpeedInterpRate() const { return AnimationSpeedInterpRate; }
	float GetAnimationTurnRateInterpRate() const { return AnimationTurnRateInterpRate; }
	bool IsBraking() const { return bBraking; }
	ERideGait GetCurrentGait() const { return CurrentGait; }

#pragma region Mount And DisMount
public:
	void DisMount();
	void DisMountInputCompleted();
	virtual void Mount(ACharacter* RiderCharacter, FVector InitVelocity);
	void ApplyRideProfile(const URideProfileDataAsset* Profile);
	void AttachRider();
	void NotifyDismountStarted(bool bMoving);
	void ReleaseRider(bool bContinueForward = false);
	virtual void FinishDismount();
	float GetRideSpeed() const;
	float GetRideDirection() const;
	FTransform GetMountTransform() const;
	FTransform GetDismountTransform() const;
	void RefreshRideCameraComponents();

protected:
	bool bDismount = false;
	bool bMovingDismount = false;
#pragma endregion

#pragma region Ride Movement
protected:
	float MaxRideSpeed = 800.0f;

	float WalkRideSpeed = 250.0f;

	float RunRideSpeed = 550.0f;

	float SprintRideSpeed = 800.0f;

	float WalkInputThreshold = 0.55f;

	float AccelerationInterpSpeed = 2.5f;

	float DecelerationInterpSpeed = 3.5f;

	float MaxTurnRate = 180.0f;

	float MinTurnRateAtMaxSpeed = 110.0f;
	float TurnRateInterpSpeed = 8.0f;
	float VelocityHeadingInterpSpeed = 5.0f;
	float FullTurnAuthoritySpeed = 120.0f;
	float MinMovingTurnAuthority = 0.0f;

	float PivotTurnMinAngle = 160.0f;

	float InputDeadZone = 0.05f;

	float MaxAnimDirection = 90.0f;
	float BlendSpaceDirectionLimit = 20.0f;
	float AnimationDirectionInterpRate = 5.0f;
	float SprintForwardAlignmentThreshold = 0.5f;
	float AnimationSpeedInterpRate = 8.0f;
	float AnimationTurnRateInterpRate = 10.0f;

	float PivotTurnMaxStartSpeed = 180.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PivotTurnMontage;

	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> PivotTurnAlphaCurve;

	bool bUseNormalizedPivotTurnCurveTime = true;

	FName PivotTurnLeftSection = TEXT("TurnLeft");

	FName PivotTurnRightSection = TEXT("TurnRight");

	bool bPivotTurning = false;
	float PivotTurnDirection = 0.0f;
	float PivotTurnStartYaw = 0.0f;
	float PivotTurnTargetDeltaYaw = 0.0f;
	float PivotTurnPreviousYaw = 0.0f;
#pragma endregion

#pragma region Need for Conversion Possess
public:
	FTransform GetCameraTransform() const;
	FTransform GetSpringArmTransform() const;
	float GetTargetArmLength() const;
	USpringArmComponent* GetRideSpringArmComponent() const { return SpringArm; }
	FRotator GetControllerRotation() const;

#pragma endregion

public:
	void PivotTurn(float TargetDeltaYaw);
};
