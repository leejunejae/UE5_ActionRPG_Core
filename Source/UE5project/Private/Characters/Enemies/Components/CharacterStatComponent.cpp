// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/Components/CharacterStatComponent.h"

bool UCharacterStatComponent::ChangeStance(const float Amount, const EStatChangeType StatChangeType)
{
	float Delta = Amount;

	switch (StatChangeType)
	{
	case EStatChangeType::Damage:
		break;
	case EStatChangeType::Heal:
		Delta *= -1.0f;
		break;
	}

	NPCStats.Stance.Current = FMath::Clamp(NPCStats.Stance.Current - Delta, 0.0f, NPCStats.Stance.Max);

	return NPCStats.Stance.Current > 0.0f;
}

void UCharacterStatComponent::BreakStance()
{
	NPCStats.Stance.Current = 0.0f;
}

void UCharacterStatComponent::RestoreStance()
{
	NPCStats.Stance.Current = NPCStats.Stance.Max;
}
