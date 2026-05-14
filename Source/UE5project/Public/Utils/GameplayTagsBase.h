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
// State & Action
// =====================

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Ground)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Ladder)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Ride)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action_Attack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action_Guard)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action_HitReact)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action_Interact)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action_Jump)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action_Dodge)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Action_Parry)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window_Attack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window_Guard)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window_HitReact)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window_Interact)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window_Jump)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window_Dodge)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Window_Parry)

// =====================
// State & Action
// =====================


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
