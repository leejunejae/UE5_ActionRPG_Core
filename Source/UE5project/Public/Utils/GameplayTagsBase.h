// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NativeGameplayTags.h"
#include "GameplayTagsBase.generated.h"

/**
 * 
 */

// =====================
// IK
// =====================

// IK.Phase
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase_Ground)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase_Ladder)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Phase_Ride)

// IK.Layer
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ground)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ground_Locomotion)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ground_HandWeapon)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ladder)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ladder_Climb)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ride)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IK_Layer_Ride_Locomotion)
// =====================
// IK
// =====================


// =====================
// Weapon
// =====================
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Unarmed)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_OneHandedSword)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_TwoHandedSword)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_SwordAndShield)
// =====================
// Weapon
// =====================

UCLASS()
class UE5PROJECT_API UGameplayTagsBase : public UObject
{
	GENERATED_BODY()
	
};
