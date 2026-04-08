// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBaseAnimInstance.h"
#include "PlayerBaseAnimInstance.generated.h"

/**
 * 
 */
class APlayerBase;

UCLASS()
class UE5PROJECT_API UPlayerBaseAnimInstance : public UCharacterBaseAnimInstance
{
	GENERATED_BODY()
	
	friend class UAnimMode_Ground_Player;
	friend class UAnimMode_Ride;

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(Transient)
		TObjectPtr<APlayerBase> Player = nullptr;

#pragma region Animation Data
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace1D> Locomotion_Normal_CycleBS = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace> Locomotion_Combat_Forward_BS = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace> Locomotion_Combat_Backward_BS = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Locomotion_Idle = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Locomotion_Start = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Locomotion_Stop_Jog = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Locomotion_Stop_Run = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Jump_Start = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Jump_Loop = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Fall_Loop = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Land_Jump = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Land_Fall = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Land_Jog = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Land_High = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> HitAir_Start = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> HitAir_Loop = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> HitAir_End = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> GetUp = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> HitAir_Death = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Ground_Death = nullptr;

	bool bInitAnimSet = false;

protected:
	void HandleWeaponChange(EWeaponType WeaponData);

#pragma endregion Animation Data

#pragma region Locomotion
protected:
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		float LeanAngle;
#pragma endregion Locomotion

#pragma region Input
private:
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ground|Locomotion", meta = (AllowPrivateAccess = "true"))
		bool IsMovementInput;
#pragma endregion Input

#pragma region Ride
public:
	FOnAnimInstanceMulDel OnMountEnd;
	FOnAnimInstanceMulDel OnDisMountEnd;

private:
	UFUNCTION()
		void AnimNotify_NOT_MountEnd();

	UFUNCTION()
		void AnimNotify_NOT_DisMountEnd();
#pragma endregion 
};
