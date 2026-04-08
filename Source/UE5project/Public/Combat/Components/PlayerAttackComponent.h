// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Components/AttackComponent.h"
#include "Combat/Data/DataAsset/PlayerAttackDataAsset.h"
#include "PlayerAttackComponent.generated.h"

/**
 * 
*/

UCLASS()
class UE5PROJECT_API UPlayerAttackComponent : public UAttackComponent
{
	GENERATED_BODY()
	
public:
	void SetCurAttackContextSet(EWeaponType WeaponData);//EWeaponType WeaponType);

	void ExecuteAttack(FName AttackName, float Playrate = 1.0f) override;

protected:
	void BeginPlay() override;

private:
};
