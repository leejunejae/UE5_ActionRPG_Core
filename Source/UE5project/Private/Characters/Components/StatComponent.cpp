// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Components/StatComponent.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "Combat/Components/HitReactionComponent.h"
#include "Utils/CoreLog.h"

// Sets default values for this component's properties
UStatComponent::UStatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.


	// ...
}

// Called when the game starts
void UStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if (UHitReactionComponent* HitComp = GetOwner()->FindComponentByClass<UHitReactionComponent>())
	{
		HitComp->HitEndDelegate.AddUObject(this, &UStatComponent::ChangePoise, GetCommonStats().GetMaxPoise(), EStatChangeType::Restore);
	}
}

void UStatComponent::ChangeMaxHealth(const float Amount)
{
	GetCommonStats().Health.Max += Amount;
}

void UStatComponent::ChangeMaxPoise(const float Amount)
{
	GetCommonStats().Poise.Max += Amount;
}

bool UStatComponent::ApplyDamage(const float Amount, const EDamageType AttackType)
{
	FCharacterStats& Stats = GetCommonStats();

	float Delta = Amount;
	switch (AttackType)
	{
	case EDamageType::PhysicalDamage:
		Delta *= (1.0f - (Stats.PhysicalDefense / (Stats.PhysicalDefense + 100.0f)));
		break;
	case EDamageType::MagicalDamage:
		Delta *= (1.0f - (Stats.MagicDefense / (Stats.MagicDefense + 100.0f)));
		break;
	case EDamageType::TrueDamage:
		break;
	}

	Stats.Health.Current = FMath::Clamp(Stats.Health.Current - Delta, 0.0f, Stats.Health.Max);
	
	if (Stats.Health.Current <= 0.0f)
	{
		OnDeath.ExecuteIfBound();
		if (UCharacterStatusComponent* StatusComp = GetOwner()->FindComponentByClass<UCharacterStatusComponent>())
		{
			StatusComp->ExecuteDeath();
		}
		return false;
	}

	return true;
}

bool UStatComponent::Heal(const float Amount)
{
	FCharacterStats& Stats = GetCommonStats();

	Stats.Health.Current = FMath::Clamp(Stats.GetHealth() + Amount, 0.0f, Stats.GetMaxHealth());

	return true;
}

void UStatComponent::ChangePoise(const float Amount, const EStatChangeType PoiseChangeType)
{
	FCharacterStats& Stats = GetCommonStats();

	float Delta = Amount;

	switch (PoiseChangeType)
	{
	case EStatChangeType::Damage:
		break;
	case EStatChangeType::Heal:
		Delta *= -1.0f;
		break;
	case EStatChangeType::Restore:
		Stats.Poise.Current = Stats.Poise.Max;
		if (AActor* Owner = GetOwner())UE_LOG(Log_Hit, Log, TEXT("[StatComponent] %s Poise has been Restored : %f"), *Owner->GetName(), Stats.Poise.Current);
		return;
	}

	Stats.Poise.Current = FMath::Clamp(Stats.Poise.Current - Delta, 0.0f, Stats.Poise.Max);

	if (Stats.Poise.Current <= 0.0f)
	{
		if(AActor* Owner = GetOwner())UE_LOG(Log_Hit, Log, TEXT("[StatComponent] %s Poise has been broken"), *Owner->GetName());
		PoiseBreakDelegate.Broadcast();
	}
}