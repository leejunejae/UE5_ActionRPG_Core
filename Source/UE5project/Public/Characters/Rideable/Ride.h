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
class URideComponent;

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
	Gallop UMETA(DisplayName = "Gallop"),
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
	bool MountRight = false;

	ACharacter* Rider;
	FRotator RiderRotator;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& value);
	void StopMove(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void UpdateRideMovement(float DeltaTime);

	bool FindMountPos();

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


	bool IsMovementInput;
	FInputActionValue MovementInputValue;
	FVector2D RideMoveInput = FVector2D::ZeroVector;
	float CurrentThrottle = 0.0f;

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
	bool IsBraking() const { return bBraking; }
	ERideGait GetCurrentGait() const { return CurrentGait; }

#pragma region Mount And DisMount
public:
	void DisMount();
	virtual bool TryDisMount();

	virtual void Mount(ACharacter* RiderCharacter, FVector InitVelocity);
	void AttachRider();
	virtual void FinishDismount();
	float GetRideSpeed() const;
	float GetRideDirection() const;
	bool GetMountDir() const;
	FTransform GetMountTransform() const;
	FTransform GetDismountTransform() const;
	bool IsMovingDismount() const;
	void RefreshRideCameraComponents();

protected:
	bool CanDismount;
	bool bDismount = false;
	bool bMovingDismount = false;
	FVector LastSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Dismount")
	float MovingDismountSpeedThreshold = 150.0f;

#pragma endregion

#pragma region Ride Movement
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float MaxRideSpeed = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float AccelerationInterpSpeed = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float DecelerationInterpSpeed = 3.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float MaxTurnRate = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float MinTurnRateAtMaxSpeed = 110.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float QuickTurnAngle = 160.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float InputDeadZone = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float DirectionInterpRate = 240.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float MaxAnimDirection = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float WalkSpeedThreshold = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ride|Movement")
	float RunSpeedThreshold = 450.0f;
#pragma endregion

#pragma region Need for Conversion Possess
public:
	FTransform GetCameraTransform() const;
	FTransform GetSpringArmTransform() const;
	float GetTargetArmLength() const;
	FRotator GetControllerRotation() const;

#pragma endregion

#pragma region Turn
protected:
	UPROPERTY(EditAnywhere, Category = Turn)
		TObjectPtr<UAnimMontage> TurnMontage;

public:
	void QuickTurn(float TurnDirection);
#pragma endregion Turn
};
