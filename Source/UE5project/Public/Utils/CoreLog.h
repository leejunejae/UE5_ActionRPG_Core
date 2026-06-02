// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoreLog.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(Log_Route, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_Check, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_Spawn_NPC, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_Anim, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_Anim_IK, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_Anim_IK_Climb, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_Anim_Equip, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_Hit, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_Attack, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_LockOn, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_RideSpawn, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_Equip_Weapon, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_AI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_AI_Task, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_AI_Task_Combat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_AI_Task_Combat_Alert, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_Character_Player, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(Log_Character_Player_Input, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_Character_Enemy, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(Log_UI, Log, All);

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UCoreLog : public UObject
{
	GENERATED_BODY()
	
};
