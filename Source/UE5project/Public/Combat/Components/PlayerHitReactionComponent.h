// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Components/HitReactionComponent.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "PlayerHitReactionComponent.generated.h"

/**
 * 
 */
class UPlayerHitReactionDataAsset;

UCLASS()
class UE5PROJECT_API UPlayerHitReactionComponent : public UHitReactionComponent
{
	GENERATED_BODY()

public:
	void HandleWeaponChange(EWeaponType WeaponType);
	FORCEINLINE void SetHitReactionListDA(UPlayerHitReactionDataAsset* InDA) { HitReactionList = InDA; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Meta = (AllowPrivateAccess = true))
	TObjectPtr<UPlayerHitReactionDataAsset> HitReactionList;
};
