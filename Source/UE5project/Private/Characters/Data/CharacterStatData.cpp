// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Data/CharacterStatData.h"

float FCharacterAttributes::GetRequirementAttributeRate(const FCharacterAttributes& Requirement) const
{
	float CorrectedStatSum = 0.0f;
	float RequireStatSum = 0.0f;

	auto ApplyStat = [&](int32 CharacterStat, int32 RequireStat) {
		if (RequireStat > 0)
		{
			RequireStatSum += RequireStat;
			CorrectedStatSum += CorrectedStatSum += FMath::Min(CharacterStat, RequireStat);
		}
	};

	ApplyStat(Vitality, Requirement.Vitality);
	ApplyStat(Endurance, Requirement.Endurance);
	ApplyStat(Mentality, Requirement.Mentality);
	ApplyStat(Strength, Requirement.Strength);
	ApplyStat(Dexterity, Requirement.Dexterity);
	ApplyStat(Intelligence, Requirement.Intelligence);
	ApplyStat(Vigor, Requirement.Vigor);

	return RequireStatSum > 0.0f ? CorrectedStatSum / RequireStatSum : 1.0f;
}