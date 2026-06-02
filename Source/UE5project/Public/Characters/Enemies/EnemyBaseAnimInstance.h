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
#pragma region Animation Dat
protected:
	void HandleDeathStarted() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true))
		TObjectPtr<UBlendSpace> Locomotion_BS = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Anim, Meta = (AllowPrivateAccess = true)) 
		TObjectPtr<UAnimMontage> DeathMontage = nullptr;
#pragma endregion Animation Data
};
