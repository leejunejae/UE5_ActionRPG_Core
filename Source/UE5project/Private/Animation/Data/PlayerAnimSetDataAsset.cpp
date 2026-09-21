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

	// 가드 애니메이션을 덮어쓴 스타일만 가드 장비 소스도 함께 덮어쓴다.
	if (!Override->Guard.IsNull()) Resolved.GuardSource = Override->GuardSource;

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

FPlayerAnimSet UPlayerAnimSetDataAsset::ResolvePlayerAnimSet(FGameplayTag CombatStyle) const
{
	const FPlayerAnimSet* Override = CombatStyleAnimList.Find(CombatStyle);
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

const FPlayerCombatStyleTransition* UPlayerAnimSetDataAsset::FindCombatStyleTransition(
	FGameplayTag FromStyle, FGameplayTag ToStyle) const
{
	return CombatStyleTransitions.FindByPredicate(
		[FromStyle, ToStyle](const FPlayerCombatStyleTransition& Transition)
		{
			return Transition.FromStyle.MatchesTagExact(FromStyle) &&
				Transition.ToStyle.MatchesTagExact(ToStyle);
		});
}
