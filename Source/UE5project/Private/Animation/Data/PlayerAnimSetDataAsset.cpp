// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Data/PlayerAnimSetDataAsset.h"

namespace
{
FPlayerAnimSet ResolveAnimSetOverride(const FPlayerAnimSet& CommonAnimSet, const FPlayerAnimSet* Override)
{
	FPlayerAnimSet Resolved = CommonAnimSet;
	if (!Override) return Resolved;

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
	APPLY_SOFT_OVERRIDE(ParryMontage)
	APPLY_SOFT_OVERRIDE(GroundDeathMontage)
	APPLY_SOFT_OVERRIDE(AirDeathMontage)
	APPLY_SOFT_OVERRIDE(LadderDeathMontage)
	APPLY_SOFT_OVERRIDE(RideDeathMontage)
	APPLY_SOFT_OVERRIDE(SpawnMontage)
#undef APPLY_SOFT_OVERRIDE

	if (!Override->CriticalExecutions.IsEmpty()) Resolved.CriticalExecutions = Override->CriticalExecutions;

#define APPLY_BLEND_OVERRIDE(Field) if (Override->DodgeExitBlendSettings.Field >= 0.0f) { Resolved.DodgeExitBlendSettings.Field = Override->DodgeExitBlendSettings.Field; }
	APPLY_BLEND_OVERRIDE(Transition)
	APPLY_BLEND_OVERRIDE(Locomotion)
	APPLY_BLEND_OVERRIDE(Interrupted)
	APPLY_BLEND_OVERRIDE(Death)
	APPLY_BLEND_OVERRIDE(EquipmentChange)
#undef APPLY_BLEND_OVERRIDE

#define APPLY_PARRY_BLEND_OVERRIDE(Field) if (Override->ParryExitBlendSettings.Field >= 0.0f) { Resolved.ParryExitBlendSettings.Field = Override->ParryExitBlendSettings.Field; }
	APPLY_PARRY_BLEND_OVERRIDE(Transition)
	APPLY_PARRY_BLEND_OVERRIDE(Locomotion)
	APPLY_PARRY_BLEND_OVERRIDE(Interrupted)
	APPLY_PARRY_BLEND_OVERRIDE(Death)
	APPLY_PARRY_BLEND_OVERRIDE(EquipmentChange)
#undef APPLY_PARRY_BLEND_OVERRIDE

	if (Override->DodgeLocomotionBlendOutTime >= 0.0f)
	{
		Resolved.DodgeLocomotionBlendOutTime = Override->DodgeLocomotionBlendOutTime;
	}
	Resolved.bUseWeaponIK = Override->bUseWeaponIK;
	return Resolved;
}
}

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
	return ResolveAnimSetOverride(CommonAnimSet, AnimList.Find(WeaponType));
}

const FPlayerAnimSet* UPlayerAnimSetDataAsset::FindPlayerAnimSet(FGameplayTag CombatStyle, bool bLogNotFound) const
{
	if (const FPlayerAnimSet* Found = CombatStyleAnimList.Find(CombatStyle)) return Found;
	return FindPlayerAnimSet(GetLegacyWeaponTypeForCombatStyle(CombatStyle), bLogNotFound);
}

FPlayerAnimSet UPlayerAnimSetDataAsset::ResolvePlayerAnimSet(FGameplayTag CombatStyle) const
{
	const FPlayerAnimSet* Override = CombatStyleAnimList.Find(CombatStyle);
	if (!Override) Override = AnimList.Find(GetLegacyWeaponTypeForCombatStyle(CombatStyle));
	return ResolveAnimSetOverride(CommonAnimSet, Override);
}

FGameplayTag UPlayerAnimSetDataAsset::ResolveCombatStyle(
	EWeaponCategory MainWeaponCategory,
	EWeaponCategory OffHandWeaponCategory,
	EWeaponGripMode GripMode) const
{
	// 정확한 주/보조/파지 조합을 가장 먼저 찾는다.
	for (const FPlayerCombatStyleRule& Rule : CombatStyleRules)
	{
		if (Rule.MainWeaponCategory == MainWeaponCategory &&
			Rule.OffHandWeaponCategory == OffHandWeaponCategory &&
			Rule.GripMode == GripMode && Rule.CombatStyle.IsValid())
		{
			return Rule.CombatStyle;
		}
	}

	// 조합이 없으면 같은 주무기와 파지 방식의 단독 규칙으로 폴백한다.
	for (const FPlayerCombatStyleRule& Rule : CombatStyleRules)
	{
		if (Rule.MainWeaponCategory == MainWeaponCategory &&
			Rule.OffHandWeaponCategory == EWeaponCategory::None &&
			Rule.GripMode == GripMode && Rule.CombatStyle.IsValid())
		{
			return Rule.CombatStyle;
		}
	}

	return FGameplayTag();
}
