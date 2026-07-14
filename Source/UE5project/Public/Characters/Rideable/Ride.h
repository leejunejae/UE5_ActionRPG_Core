// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 엔진 헤더
#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// 입력
#include "InputActionValue.h"

// 구조체, 자료형


// 인터페이스
#include "Characters/Rideable/Interfaces/RideInterface.h"
#include "Characters/Player/Interfaces/ViewDataInterface.h"

// 태그
#include "GameplayTagContainer.h"


#include "Ride.generated.h"

class UCharacterMovementComponent;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;

UENUM(BlueprintType)
enum class HorseDirection : uint8
{
	Back UMETA(DisplayName = "Back"),
	Stop UMETA(DisplayName = "Stop"),
	Front UMETA(DisplayName = "Front"),
};

UCLASS()
class UE5PROJECT_API ARide : public ACharacter, public IRideInterface, public IViewDataInterface
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
	bool MountRight = false;

	ACharacter* Rider;
	FRotator RiderRotator;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);

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

	float GetDirection();

#pragma region Mount And DisMount
public:
	void DisMount();
	virtual bool TryDisMount();

	virtual void Mount_Implementation(ACharacter* RiderCharacter, FVector InitVelocity);
	virtual float GetRideSpeed_Implementation();
	virtual float GetRideDirection_Implementation();
	virtual bool GetMountDir_Implementation();
	virtual FTransform GetMountTransform_Implementation();

protected:
	bool CanDismount;
	bool bDismount = false;
	FVector LastSpeed;

#pragma endregion

#pragma region Need for Conversion Possess
private:
	void CameraSettingTimer();
	FTimerHandle CameraSettingTimerHandle;
	float SpringArmLength = 200.0f;

public:
	virtual FTransform GetCameraTransform_Implementation();
	virtual FTransform GetSpringArmTransform_Implementation();
	virtual float GetTargetArmLength_Implementation();
	virtual FRotator GetControllerRotation_Implementation();

#pragma endregion

#pragma region Turn
protected:
	UPROPERTY(EditAnywhere, Category = Turn)
		TObjectPtr<UAnimMontage> TurnMontage;

public:
	void QuickTurn(float TurnDirection);
#pragma endregion Turn
};
