// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/GameplayTagsBase.h"

// =====================
// State & Action
// =====================

UE_DEFINE_GAMEPLAY_TAG(TAG_State, "State")

UE_DEFINE_GAMEPLAY_TAG(TAG_State_Ground, "State.Ground")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Ladder, "State.Ladder")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Ride, "State.Ride")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Dead, "State.Dead")

UE_DEFINE_GAMEPLAY_TAG(TAG_Action, "Action")

UE_DEFINE_GAMEPLAY_TAG(TAG_Action_Attack, "Action.Attack")
UE_DEFINE_GAMEPLAY_TAG(TAG_Action_Guard, "Action.Guard")
UE_DEFINE_GAMEPLAY_TAG(TAG_Action_HitReact, "Action.HitReact")
UE_DEFINE_GAMEPLAY_TAG(TAG_Action_Interact, "Action.Interact")
UE_DEFINE_GAMEPLAY_TAG(TAG_Action_Jump, "Action.Jump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Action_Dodge, "Action.Dodge")
UE_DEFINE_GAMEPLAY_TAG(TAG_Action_Parry, "Action.Parry")

UE_DEFINE_GAMEPLAY_TAG(TAG_Window, "Window")

UE_DEFINE_GAMEPLAY_TAG(TAG_Window_Attack, "Window.Attack")
UE_DEFINE_GAMEPLAY_TAG(TAG_Window_Guard, "Window.Guard")
UE_DEFINE_GAMEPLAY_TAG(TAG_Window_HitReact, "Window.HitReact")
UE_DEFINE_GAMEPLAY_TAG(TAG_Window_Interact, "Window.Interact")
UE_DEFINE_GAMEPLAY_TAG(TAG_Window_Jump, "Window.Jump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Window_Dodge, "Window.Dodge")
UE_DEFINE_GAMEPLAY_TAG(TAG_Window_Parry, "Window.Parry")

// =====================
// State & Action
// =====================


// =====================
// IK
// =====================
// 
// IK.Phase
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Phase, "IK.Phase")
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Phase_Ground, "IK.Phase.Ground")
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Phase_Ladder, "IK.Phase.Ladder")
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Phase_Ride, "IK.Phase.Ride")

// IK.Layer
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer, "IK.Layer")

UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer_Ground, "IK.Layer.Ground")
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer_Ground_Locomotion, "IK.Layer.Ground.Locomotion")
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer_Ground_HandWeapon, "IK.Layer.Ground.HandWeapon")

UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer_Ladder, "IK.Layer.Ladder")
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer_Ladder_Climb, "IK.Layer.Ladder.Climb")

UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer_Ride, "IK.Layer.Ride")
UE_DEFINE_GAMEPLAY_TAG(TAG_IK_Layer_Ride_Locomotion, "IK.Layer.Ride.Locomotion")

// =====================
// IK
// =====================


// =====================
// Weapon
// =====================

UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Unarmed, "Weapon.Unarmed")
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_OneHandedSword, "Weapon.OneHandedSword")
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_TwoHandedSword, "Weapon.TwoHandedSword")
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_SwordAndShield, "Weapon.SwordAndShield")

// =====================
// Weapon
// =====================