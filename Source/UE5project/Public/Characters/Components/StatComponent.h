// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/Data/CharacterStatData.h"
#include "Combat/Data/CombatData.h"

#include "StatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnStatCompMultiDel);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnResourceStatChanged, EResourceStatType, float, float);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5PROJECT_API UStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	virtual FCharacterStats& GetCommonStats() { return BaseStats; }

	FOnStatCompMultiDel PoiseBreakDelegate;

	FOnResourceStatChanged OnResourceStatChanged;

public:
	void ChangeMaxHealth(const float Amount);
	void ChangeMaxPoise(const float Amount);
	bool ApplyDamage(const float Amount, const EDamageType AttackType);
	bool Heal(const float Amount);
	void ChangePoise(const float Amount, const EStatChangeType PoiseChangeType);

	void BroadcastResourceStat(EResourceStatType StatType, const FResourceStat& Resource);

protected:
	FCharacterStats BaseStats;
};