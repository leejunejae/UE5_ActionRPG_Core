// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Data/PlayerAnimSetDataAsset.h"

const FPlayerAnimSet* UPlayerAnimSetDataAsset::FindPlayerAnimSet(const EWeaponType& WeaponType, bool bLogNotFound) const
{
	const FPlayerAnimSet* Found = AnimList.Find(WeaponType);
	if (Found)
	{
		return Found;
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Not SkillInfo"))
	}

	return nullptr;
}

FPlayerAnimSet UPlayerAnimSetDataAsset::ResolvePlayerAnimSet(const EWeaponType& WeaponType) const
{
	FPlayerAnimSet Resolved = CommonAnimSet;
	const FPlayerAnimSet* Override = AnimList.Find(WeaponType);
	if (!Override)
	{
		return Resolved;
	}

#define APPLY_SOFT_OVERRIDE(Field) if (!Override->Field.IsNull()) { Resolved.Field = Override->Field; }
	APPLY_SOFT_OVERRIDE(Locomotion_Normal_CycleBS)
	APPLY_SOFT_OVERRIDE(Locomotion_Combat_Forward_BS)
	APPLY_SOFT_OVERRIDE(Locomotion_Combat_Backward_BS)
	APPLY_SOFT_OVERRIDE(Locomotion_Idle)
	APPLY_SOFT_OVERRIDE(Locomotion_Start)
	APPLY_SOFT_OVERRIDE(Locomotion_Stop_Jog)
	APPLY_SOFT_OVERRIDE(Locomotion_Stop_Run)
	APPLY_SOFT_OVERRIDE(Jump_Start)
	APPLY_SOFT_OVERRIDE(Jump_Loop)
	APPLY_SOFT_OVERRIDE(Fall_Loop)
	APPLY_SOFT_OVERRIDE(Land_Jump)
	APPLY_SOFT_OVERRIDE(Land_Fall)
	APPLY_SOFT_OVERRIDE(Land_Jog)
	APPLY_SOFT_OVERRIDE(Land_High)
	APPLY_SOFT_OVERRIDE(HitAir_Start)
	APPLY_SOFT_OVERRIDE(HitAir_Loop)
	APPLY_SOFT_OVERRIDE(HitAir_End)
	APPLY_SOFT_OVERRIDE(GetUp)
	APPLY_SOFT_OVERRIDE(Guard)
	APPLY_SOFT_OVERRIDE(DodgeMontage)
	APPLY_SOFT_OVERRIDE(GroundDeathMontage)
	APPLY_SOFT_OVERRIDE(AirDeathMontage)
	APPLY_SOFT_OVERRIDE(LadderDeathMontage)
	APPLY_SOFT_OVERRIDE(RideDeathMontage)
	APPLY_SOFT_OVERRIDE(SpawnMontage)
#undef APPLY_SOFT_OVERRIDE

#define APPLY_BLEND_OVERRIDE(Field) \
	if (Override->DodgeExitBlendSettings.Field >= 0.0f) \
	{ Resolved.DodgeExitBlendSettings.Field = Override->DodgeExitBlendSettings.Field; }
	APPLY_BLEND_OVERRIDE(Transition)
	APPLY_BLEND_OVERRIDE(Locomotion)
	APPLY_BLEND_OVERRIDE(Interrupted)
	APPLY_BLEND_OVERRIDE(Death)
	APPLY_BLEND_OVERRIDE(EquipmentChange)
#undef APPLY_BLEND_OVERRIDE

	if (Override->DodgeLocomotionBlendOutTime >= 0.0f)
	{
		Resolved.DodgeLocomotionBlendOutTime = Override->DodgeLocomotionBlendOutTime;
	}

	// IK 사용 여부는 무기 자체의 명시적 특성이므로 무기 항목이 있으면 그대로 사용한다.
	Resolved.bUseWeaponIK = Override->bUseWeaponIK;
	return Resolved;
}
