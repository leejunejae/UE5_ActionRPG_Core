// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Weapons/Data/WeaponData.h"
#include "Utils/GameplayTagsBase.h"

EWeaponType GetLegacyWeaponTypeForCombatStyle(FGameplayTag CombatStyle)
{
	if (CombatStyle.MatchesTagExact(TAG_CombatStyle_Sword_Shield)) return EWeaponType::SwordAndShield;
	if (CombatStyle.MatchesTagExact(TAG_CombatStyle_Sword_TwoHanded)) return EWeaponType::LongSword;
	if (CombatStyle.MatchesTagExact(TAG_CombatStyle_GreatSword_TwoHanded)) return EWeaponType::GreatSword;
	if (CombatStyle.MatchesTagExact(TAG_CombatStyle_Spear_Shield)) return EWeaponType::SpearAndShield;
	if (CombatStyle.MatchesTagExact(TAG_CombatStyle_Fist)) return EWeaponType::Knuckles;
	return EWeaponType::None;
}

FGameplayTag GetLegacyCombatStyleForWeaponType(EWeaponType WeaponType)
{
	switch (WeaponType)
	{
	case EWeaponType::SwordAndShield: return TAG_CombatStyle_Sword_Shield;
	case EWeaponType::LongSword: return TAG_CombatStyle_Sword_TwoHanded;
	case EWeaponType::GreatSword: return TAG_CombatStyle_GreatSword_TwoHanded;
	case EWeaponType::SpearAndShield: return TAG_CombatStyle_Spear_Shield;
	case EWeaponType::Knuckles: return TAG_CombatStyle_Fist;
	default: return TAG_CombatStyle_Unarmed;
	}
}

EWeaponCategory GetWeaponCategoryFromLegacyType(EWeaponType WeaponType)
{
	switch (WeaponType)
	{
	case EWeaponType::SwordAndShield:
	case EWeaponType::LongSword: return EWeaponCategory::Sword;
	case EWeaponType::GreatSword: return EWeaponCategory::GreatSword;
	case EWeaponType::SpearAndShield: return EWeaponCategory::Spear;
	case EWeaponType::Knuckles: return EWeaponCategory::FistWeapon;
	default: return EWeaponCategory::None;
	}
}
