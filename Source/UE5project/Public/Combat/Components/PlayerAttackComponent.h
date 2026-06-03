// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Components/AttackComponent.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "PlayerAttackComponent.generated.h"

class UPlayerAttackDataAsset;

UCLASS()
class UE5PROJECT_API UPlayerAttackComponent : public UAttackComponent
{
	GENERATED_BODY()
	
public:
	void SetCurAttackContextSet(EWeaponType WeaponType);

	const FBaseAttackData* ExecuteAttack(FName AttackName, float Playrate = 1.0f) override;

	FORCEINLINE void SetAttackDA(UPlayerAttackDataAsset* AttackDA) { AttackList = AttackDA; }

protected:
	void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
	TObjectPtr<UPlayerAttackDataAsset> AttackList;
};
