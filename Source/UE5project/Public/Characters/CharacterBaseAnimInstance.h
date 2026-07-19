// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 엔진 헤더
#include "CoreMinimal.h"

// 구조체, 자료형

#include "Characters/Data/BaseCharacterHeader.h"
#include "Combat/Data/CombatData.h"
#include "Characters/Data/StatusData.h"
#include "Characters/Rideable/Ride.h"
#include "Interaction/Climb/Data/ClimbHeader.h"
#include "Characters/Data/IKData.h"
#include "Animation/Data/AnimData.h"
#include "Items/Weapons/Data/WeaponData.h"

// 인터페이스
#include "Animation/Interfaces/IAnimInstance.h"

// 애님모드
#include "Animation/Mode/AnimModeBase.h"

#include "Animation/AnimInstance.h"
#include "CharacterBaseAnimInstance.generated.h"

DECLARE_DELEGATE(FOnSingleDelegate);
DECLARE_MULTICAST_DELEGATE(FOnAnimInstanceMulDel);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMulOneParamDelegate, FName);

class ACharacterBase;

UCLASS()
class UE5PROJECT_API UCharacterBaseAnimInstance : public UAnimInstance, public IIAnimInstance
{
	GENERATED_BODY()

	friend class UAnimModeBase;
	friend class UAnimMode_Ground;
	friend class UAnimMode_Ground_Player;
	friend class UAnimMode_Ground_NPC;
	friend class UAnimMode_Ladder;
	friend class UAnimMode_Ride;

public:
	UCharacterBaseAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(Transient)
		TObjectPtr<ACharacterBase> Character = nullptr;

#pragma region Animation Mode
protected:
	UPROPERTY(VisibleAnywhere, Transient)
		TMap<FGameplayTag, UAnimModeBase*> AnimModeMap;
	
	UPROPERTY(VisibleAnywhere)
		TObjectPtr<UAnimModeBase> CurrentMode = nullptr;

	void SwitchAnimMode(const FGameplayTag TargetMode);

#pragma endregion Animation Mode

#pragma region State & Stance_IK
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = IK)
		TMap<FGameplayTag, float> IKPhase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = IK)
		TMap<FGameplayTag, FIKContextWeights> IKLayer;

public:
	void SetIKPhaseAlpha_Native(FGameplayTag TargetIKPhase, float Weight);
	float GetIKPhaseAlpha_Native(FGameplayTag TargetIKPhase);
	void SetIKLayerAlpha_Native(FGameplayTag TargetIKLayer, ELimbList Limb, float Weight);
	float GetIKLayerAlpha_Native(FGameplayTag TargetIKLayer, ELimbList Limb);

	void SetIKPhaseAlpha_Implementation(FGameplayTag TargetIKPhase, float Weight) override;
	float GetIKPhaseAlpha_Implementation(FGameplayTag TargetIKPhase) override;
	void SetIKLayerAlpha_Implementation(FGameplayTag TargetIKLayer, ELimbList Limb, float Weight) override;
	float GetIKLayerAlpha_Implementation(FGameplayTag TargetIKLayer, ELimbList Limb) override;
#pragma endregion State & Stance_IK

private:
//	UFUNCTION(BlueprintCallable)
		//void AnimNotify_NOT_EnterWalkState();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = State, Meta = (AllowPrivateAccess = true))
		FGameplayTag CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Stance, Meta = (AllowPrivateAccess = true))
		EClimbPhase CurLadderStance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Stance, Meta = (AllowPrivateAccess = true))
		ERideAnimPhase CurRideAnimPhase;

public:
	FOnAnimInstanceMulDel OnEnterWalkState;
	FOnAnimInstanceMulDel OnEnterLadderState;

#pragma region Ground

#pragma region Locomotion

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Stance, Meta = (AllowPrivateAccess = true))
		ELocomotionGait CurLocomotionGait;

#pragma endregion Locomotion

private:
	UFUNCTION(BlueprintCallable)
		void AnimNotify_NOT_EnableRootLock();
	UFUNCTION(BlueprintCallable)
		void AnimNotify_NOT_DisableRootLock();

protected:
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		float Speed;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		float Direction;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		float LocomotionAnimTime;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		float LastDirection;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		float LastSpeed;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		bool IsAccelerating;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Jump", meta = (AllowPrivateAccess = "true"))
		bool IsInAir;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Jump", meta = (AllowPrivateAccess = "true"))
		bool IsJumping;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Jump", meta = (AllowPrivateAccess = "true"))
		bool IsFalling;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Jump", meta = (AllowPrivateAccess = "true"))
		bool IsLanding;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Jump", meta = (AllowPrivateAccess = "true"))
		bool bSkipJumpStart;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ride|Locomotion", meta = (AllowPrivateAccess = "true"))
		float RideTurnRate;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ride|Locomotion", meta = (AllowPrivateAccess = "true"))
		bool bRideBraking;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ride|Locomotion", meta = (AllowPrivateAccess = "true"))
		ERideGait RideGait = ERideGait::Idle;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Combat", meta = (AllowPrivateAccess = "true"))
		bool bIsLockOn;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Combat", meta = (AllowPrivateAccess = "true"))
		float GuardBlend;

	// Foot IK //
	TTuple<FVector, float> FootTrace(FName SocketName);
	void FootRotation(float DeltaTime, FVector TargetNormal, FRotator* FootRotator, float fInterpSpeed);

#pragma region Ground_IK
protected:
	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		FRotator LeftFootRotator;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		FRotator RightFootRotator;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		float PelvisOffset;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		FVector LeftFootGroundOffset;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		FVector RightFootGroundOffset;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		FVector LeftHandWeaponOffset;

#pragma endregion Ground_IK

#pragma region Ground_Movement
	////////////////////////////////////
	// Variables For FootIK
	////////////////////////////////////
private:
	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Movement, Meta = (AllowPrivateAccess = true))
		float MovementAlpha;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Movement, Meta = (AllowPrivateAccess = true))
		EDirection8Way CurrentDirection;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Movement, Meta = (AllowPrivateAccess = true))
		bool CanMovementInput = true;

	//float CharacterYaw;
	//float GetAnimDirection(float DeltaSeconds);

	//bool IsFirstUpdateYaw;

#pragma endregion Ground_Movement

#pragma endregion Ground


#pragma region Ladder

#pragma region Ladder_IK
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		FVector LeftFootLadderOffset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FootIK, Meta = (AllowPrivateAccess = true))
		FVector RightFootLadderOffset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = HandIK, Meta = (AllowPrivateAccess = true))
		FVector LeftHandLadderOffset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = HandIK, Meta = (AllowPrivateAccess = true))
		FVector RightHandLadderOffset;
#pragma endregion Ladder_IK

protected:
	UFUNCTION(BlueprintCallable)
	void AnimNotify_NOT_ResetClimbState();
#pragma endregion Ladder
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = IK_Ride, Meta = (AllowPrivateAccess = true))
	FVector IK_FootL_Ride_Locomotion;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = IK_Ride, Meta = (AllowPrivateAccess = true))
	FVector IK_FootR_Ride_Locomotion;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = IK_Ride, Meta = (AllowPrivateAccess = true))
	FVector IK_HandL_Ride_Locomotion;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = IK_Ride, Meta = (AllowPrivateAccess = true))
	FVector IK_HandR_Ride_Locomotion;

#pragma region Ride_IK
private:

#pragma endregion RIde_IK

#pragma endregion State & Stance


#pragma region HitReaction
public:
	void SetHitAir(bool HitState);
	void ResetHitAir_Implementation() override;

private:
	UPROPERTY(BlueprintReadOnly, Category = HitReaction, Meta = (AllowPrivateAccess = true))
		bool bIsHitAir = false;

#pragma endregion HitReaction

#pragma region Death
protected:
	virtual void HandleDeathStarted();
#pragma endregion Death
};
