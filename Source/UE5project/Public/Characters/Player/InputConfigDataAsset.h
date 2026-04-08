// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputConfigDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UInputConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputMappingContext> DefaultContext;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Move;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Look;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Dodge;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Walk;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Sprint;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> CheckMove;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Jump;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Attack;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Block;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Parry;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Interact;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> SpawnRide;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> Modifier;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> LockOn;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> LockOnSwitchLeft;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> LockOnSwitchRight;
};
