// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBaseAnimInstance.h"
#include "EnemyBaseAnimInstance.generated.h"

/**
 * 
 */
class AEnemyBase;

UCLASS()
class UE5PROJECT_API UEnemyBaseAnimInstance : public UCharacterBaseAnimInstance
{
	GENERATED_BODY()
	
public:
	UEnemyBaseAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void InitAnimationData(FAnimDataSet AnimData);

protected:
protected:
	UPROPERTY(Transient)
		TObjectPtr<AEnemyBase> Enemy = nullptr;
#pragma region Animation Data
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		FAnimDataSet Locomotion;
	/*
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace> Locomotion_BS = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UAnimSequence> Locomotion_Idle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TMap<FPhaseAnimByCardinal Locomotion_Phase;

	/*
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace1D> Locomotion_WalkSlowBS = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace1D> Locomotion_WalkBS = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace1D> Locomotion_JogBS = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace1D> Locomotion_RunBS = nullptr;
	
		
		*/
#pragma endregion Animation Data
};
