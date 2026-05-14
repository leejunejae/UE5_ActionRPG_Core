// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBase.h"
#include "InputActionValue.h"

// 구조체, 자료형
#include "Characters/Data/BaseCharacterHeader.h"

// 인터페이스
#include "Characters/Player/Interfaces/PlayerInterface.h"
#include "Combat/Interfaces/HitReactionInterface.h"
#include "Combat/Interfaces/AttackSourceInterface.h"
#include "Characters/Player/Interfaces/ViewDataInterface.h"
#include "Characters/Interfaces/CharacterTransformInterface.h"
#include "PlayerBase.generated.h"

class UInputMappingContext;
class UInputAction;

class UCameraComponent;
class USpringArmComponent;
class UCharacterMovementComponent;
class UBaseCharacterMovementComponent;

class UInputConfigDataAsset;

class UPlayerBaseAnimInstance;
class UPlayerStatusComponent;
class UPlayerStatComponent;
class UEquipmentComponent;
class UCombatComponent;
class UPlayerAttackComponent;
class UPlayerHitReactionComponent;
class UInteractComponent;
class ULockOnComponent;

class UPlayerConfig;

class UDefaultWidget;
class APlayerRide;

struct FGameplayTag;

DECLARE_DELEGATE(FOnSingleDelegate);

UCLASS()
class UE5PROJECT_API APlayerBase : public ACharacterBase,
	public IPlayerInterface,
	public IViewDataInterface,
	public ICharacterTransformInterface,
	public IAttackSourceInterface
{
	GENERATED_BODY()

public:
	APlayerBase(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;

	/* ============================================================
	 *  Private Variables
	 * ============================================================ */
private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UDefaultWidget> DefaultWidgetClass;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UDefaultWidget> DefaultWidget;

	float YAxisScale;
	float DebugUpdateInterval = 0.1f;
	float TimeSinceLastDebugUpdate = 0.0f;
	FVector PastLastInputVector;

	/* ============================================================
	 *  Protected Variables
	 * ============================================================ */
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement)
	class UBaseCharacterMovementComponent* BaseMovement;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<UCameraComponent> Camera;

	FVector2D AimOffVal;

	bool IsLocomotion = true;
	bool IsMovementInput;

	FVector InputVector;
	FVector DodgeVector;

	bool IsInteraction;

	FVector InitSpringArmLocation;

	/* ============================================================
	 *  Input — 기본 이동/카메라
	 * ============================================================ */
protected:
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);

	void StartMoveInput();
	void EndMoveInput();

	void CameraSetting();
	void Landed(const FHitResult& Hit) override;

	/* ============================================================
	 *  Public Utility
	 * ============================================================ */
public:
	virtual bool IsPlayer_Implementation();
	virtual TOptional<FVector> GetCharBoneLocation(FName BoneName);

	bool GetIsMovementInput();
	float GetRideSpeed();
	float GetRideDirection();
	FVector GetInputDirection();

	/* ============================================================
	 *  Config
	 * ============================================================ */
#pragma region Init Data
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UPlayerConfig> Config;

	void ApplyConfig();
#pragma endregion

	/* ============================================================
	 *  Input Config & Modifier
	 * ============================================================ */
#pragma region Input
private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputConfigDataAsset> InputConfig;

	FORCEINLINE void ModifierInput() { IsModifierInput = true; }
	FORCEINLINE void ModifierInputEnd() { IsModifierInput = false; }

	bool IsModifierInput = false;
#pragma endregion Input

	/* ============================================================
	 *  Stat
	 * ============================================================ */
#pragma region Status
private:
	UPROPERTY(VisibleAnywhere, Category = Stat)
	TObjectPtr<UPlayerStatComponent> StatComponent;

	bool IsBlockInput = false;

public:
	FORCEINLINE UPlayerStatComponent* GetStatComponent() const { return StatComponent; }
#pragma endregion Status

	/* ============================================================
	 *  Equipment
	 * ============================================================ */
#pragma region Inventory & Equip
private:
	UPROPERTY(VisibleAnywhere, Category = Equip)
	TObjectPtr<UEquipmentComponent> EquipmentComponent;

public:
	FORCEINLINE UEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	UStaticMeshComponent* GetMainWeaponMesh() const override;

	virtual FAttackTraceSource GetAttackTraceSource(EAttackSourceType AttackSourceType) const override;
	virtual FAttackDamageSource GetAttackDamageSource() const override;
#pragma endregion Inventory & Equip

	/* ============================================================
	 *  Animation
	 * ============================================================ */
#pragma region Animation
protected:
	UPROPERTY(VisibleAnywhere, Category = Animation)
	TObjectPtr<UPlayerBaseAnimInstance> CharacterBaseAnim;
#pragma endregion Animation

	/* ============================================================
	 *  State & Stance
	 * ============================================================ */
#pragma region State & Stance
protected:
	ERideStance CurRideStance = ERideStance::Riding;

public:
	ERideStance GetCurRideStance();
#pragma endregion State & Stance

	/* ============================================================
	 *  Ground Movement (Walk / Jog / Sprint)
	 * ============================================================ */
#pragma region Ground
protected:
	virtual void Walk();
	virtual void Jog();
	virtual void Sprint();

public:
	float GetDirection();
	void SetRotationInputDirection_Implementation();

private:
	float Direction;

	FRotator InputRotation;
	bool bForcedRotatingInputDirection = false;
	float ForcedRotationSpeed = 720.0f;

	UAnimMontage* RollMontage;
#pragma endregion Ground

	/* ============================================================
	 *  Character Bone
	 * ============================================================ */
#pragma region Character Bone
public:
	TOptional<FVector> GetRideIKTargetLoc(EBodyType BoneType);
#pragma endregion

	/* ============================================================
	 *  Ride
	 * ============================================================ */
#pragma region Ride
private:
	void MountTimer();
	FTimerHandle MountTimerHandle;

	void CameraSettingTimer();
	FTimerHandle CameraSettingTimerHandle;

	void JumpDismountTimer();
	FTimerHandle JumpDismountTimerHandle;

	// ---- 입력 → 판단 ----
	void SpawnRideInput();

	// ---- 실행 ----
	void ExecuteSpawnRide();

protected:
	void MountEnd();
	void DisMountEnd();

	UPROPERTY(EditDefaultsOnly, Category = "Ride")
	TSubclassOf<APlayerRide> RideClass;

public:
	virtual FTransform GetCameraTransform_Implementation();
	virtual FTransform GetSpringArmTransform_Implementation();
	virtual float GetTargetArmLength_Implementation();
	virtual FRotator GetControllerRotation_Implementation();
	void DespawnRide_Implementation(FVector InitVelocity);

#pragma endregion Ride

	/* ============================================================
	 *  Status Component
	 * ============================================================ */
#pragma region Status
public:
	FORCEINLINE UPlayerStatusComponent* GetCharacterStatusComponent() const;

	void OnActionFinished(bool bInterrupted);
	void OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);
#pragma endregion Status

	/* ============================================================
	 *  Interact
	 * ============================================================ */
#pragma region Interact
protected:
	void HandleArrivedInteractionPoint();

	UPROPERTY(VisibleAnywhere, Category = Interact)
	TObjectPtr<UInteractComponent> InteractComponent;

public:
	virtual void RegisterInteractableActor_Implementation(AActor* Interactable);
	virtual void DeRegisterInteractableActor_Implementation(AActor* Interactable);
	virtual void EndInteraction_Implementation(AActor* Interactable);

	FORCEINLINE UInteractComponent* GetInteractComponent() const { return InteractComponent; }
#pragma endregion

	/* ============================================================
	 *  Combat — 입력(판단) / 실행 분리
	 * ============================================================ */
#pragma region Combat
private:
	UPROPERTY(VisibleAnywhere, Category = Combat)
	TObjectPtr<UCombatComponent> CombatComponent;

	// ---- 입력 → 판단 (RequestAction) ----
	void AttackInput();
	void AttackInputEnd();

	void JumpInput();
	void DodgeInput();
	void BlockInput();
	void BlockInputEnd();
	void InteractInput();
	//void ParryInput();

	// ---- 실행 (순수 로직, 판단 없음) ----
	void ExecuteAttack();
	void ExecuteJump();
	void ExecuteDodge();
	void ExecuteBlock();
	void ExecuteInteract();

	// ---- 버퍼 소비 콜백 ----
	void HandleBufferedAction(const FGameplayTag& ActionTag);

	bool IsAttackInput;

	FTimerHandle AttackTimerHandle;
	float AttackChargeTime = 0.0f;

public:
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; }
	FORCEINLINE UPlayerAttackComponent* GetAttackComponent() const;

#pragma region HitReaction
public:
	virtual void OnHit_Implementation(const FAttackRequest& AttackInfo) override;
	virtual void OnDeathEnd_Implementation() override;
	virtual void OnDeath();

	FORCEINLINE UPlayerHitReactionComponent* GetHitReactionComponent() const;
#pragma endregion HitReaction

#pragma endregion Combat

	/* ============================================================
	 *  LockOn
	 * ============================================================ */
#pragma region LockOn
private:
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<UInputAction> LockOnAction;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<UInputAction> LockOnSwitchLeftAction;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<UInputAction> LockOnSwitchRightAction;

	UPROPERTY(VisibleAnywhere, Category = "LockOn")
	TObjectPtr<ULockOnComponent> LockOnComponent;

	UPROPERTY(EditAnywhere, Category = "LockOn")
	float LockOnTurnInterpSpeed = 12.f;

	void ApplyLockOnRotation(float DeltaTime);
	void SetLockOnMovementMode(bool bLockOn);
	void OnLockOnToggle();
	void OnLockOnSwitchLeft();
	void OnLockOnSwitchRight();

public:
	FORCEINLINE ULockOnComponent* GetLockOnComponent() const { return LockOnComponent; }
#pragma endregion LockOn
};
