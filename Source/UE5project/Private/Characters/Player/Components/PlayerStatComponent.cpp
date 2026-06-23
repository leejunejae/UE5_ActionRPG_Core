// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/Components/PlayerStatComponent.h"
#include "Utils/CoreLog.h"

UPlayerStatComponent::UPlayerStatComponent()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> VitalityDT_Asset(TEXT("DataTable'/Game/00_Character/Data/Stat/Attribute_Vitality_DT.Attribute_Vitality_DT'"));
	if (VitalityDT_Asset.Succeeded())
	{
		AttributeTables.Add(EAttributeType::Vitality, VitalityDT_Asset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> EnduranceDT_Asset(TEXT("DataTable'/Game/00_Character/Data/Stat/Attribute_Endurance_DT.Attribute_Endurance_DT'"));
	if (EnduranceDT_Asset.Succeeded())
	{
		AttributeTables.Add(EAttributeType::Endurance, EnduranceDT_Asset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> MentalityDT_Asset(TEXT("DataTable'/Game/00_Character/Data/Stat/Attribute_Mentality_DT.Attribute_Mentality_DT'"));
	if (MentalityDT_Asset.Succeeded())
	{
		AttributeTables.Add(EAttributeType::Mentality, MentalityDT_Asset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> StrengthDT_Asset(TEXT("DataTable'/Game/00_Character/Data/Stat/Attribute_Strength_DT.Attribute_Strength_DT'"));
	if (StrengthDT_Asset.Succeeded())
	{
		AttributeTables.Add(EAttributeType::Strength, StrengthDT_Asset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> DexterityDT_Asset(TEXT("DataTable'/Game/00_Character/Data/Stat/Attribute_Dexterity_DT.Attribute_Dexterity_DT'"));
	if (DexterityDT_Asset.Succeeded())
	{
		AttributeTables.Add(EAttributeType::Dexterity, DexterityDT_Asset.Object);
	}
}

void UPlayerStatComponent::InitializeStats()
{
	if (AttributeTables.Contains(EAttributeType::Vitality))
	{
		FString RowName = FString::Printf(TEXT("Vitality_%d"), BaseAttributes.Vitality);
		const FAttribute_Vitality* VitalityDTRow = AttributeTables[EAttributeType::Vitality]->FindRow<FAttribute_Vitality>(FName(*RowName), TEXT(""));

		if (VitalityDTRow)
		{
			PlayerStats.BaseStats.Health.Max = VitalityDTRow->HP;
			PlayerStats.BaseStats.Health.Current = PlayerStats.BaseStats.Health.Max;
		}
		else
		{
			UE_LOG(Log_Character_Player_Stat, Warning, TEXT("Vitality row not found: %s"), *RowName);
		}
	}

	if (AttributeTables.Contains(EAttributeType::Endurance))
	{
		FString RowName = FString::Printf(TEXT("Endurance_%d"), BaseAttributes.Endurance);
		const FAttribute_Endurance* EnduranceDTRow = AttributeTables[EAttributeType::Endurance]->FindRow<FAttribute_Endurance>(FName(*RowName), TEXT(""));

		if (EnduranceDTRow)
		{
			PlayerStats.Stamina.Max = EnduranceDTRow->Stamina;
			PlayerStats.Stamina.Current = PlayerStats.Stamina.Max;
		}
		else
		{
			UE_LOG(Log_Character_Player_Stat, Warning, TEXT("Endurance row not found: %s"), *RowName);
		}
	}

	if (AttributeTables.Contains(EAttributeType::Mentality))
	{
		FString RowName = FString::Printf(TEXT("Mentality_%d"), BaseAttributes.Mentality);
		const FAttribute_Mentality* MentalityDTRow = AttributeTables[EAttributeType::Mentality]->FindRow<FAttribute_Mentality>(FName(*RowName), TEXT(""));

		if (MentalityDTRow)
		{
			PlayerStats.Focus.Max = MentalityDTRow->Focus;
			PlayerStats.Focus.Current = PlayerStats.Focus.Max;
		}
		else
		{
			UE_LOG(Log_Character_Player_Stat, Warning, TEXT("Mentality row not found: %s"), *RowName);
		}
	}

	if (AttributeTables.Contains(EAttributeType::Strength))
	{
		FString RowName = FString::Printf(TEXT("Strength_%d"), BaseAttributes.Strength);
		const FAttribute_Strength* Row = AttributeTables[EAttributeType::Strength]->FindRow<FAttribute_Strength>(FName(*RowName), TEXT(""));
		if (Row)
		{
			PlayerStats.CombatStats.StrengthAttackBonus = Row->PhysicalAttackBonus;
			PlayerStats.BaseStats.Poise.InitResource(Row->PoiseMax);
			PlayerStats.EquipLoad.Max = Row->EquipLoadMax;
		}
		else
		{
			UE_LOG(Log_Character_Player_Stat, Warning, TEXT("Strength row not found: %s"), *RowName);
		}
	}

	if (AttributeTables.Contains(EAttributeType::Dexterity))
	{
		FString RowName = FString::Printf(TEXT("Dexterity_%d"), BaseAttributes.Dexterity);
		const FAttribute_Dexterity* Row = AttributeTables[EAttributeType::Dexterity]->FindRow<FAttribute_Dexterity>(FName(*RowName), TEXT(""));
		if (Row)
		{
			PlayerStats.CombatStats.DexterityAttackBonus = Row->PhysicalAttackBonus;
			PlayerStats.CombatStats.StaminaRegenBonus = Row->StaminaRegenBonus;
		}
		else
		{
			UE_LOG(Log_Character_Player_Stat, Warning, TEXT("Dexterity row not found: %s"), *RowName);
		}
	}

	if (!AttributeTables.Contains(EAttributeType::Affinity))
	{
		UE_LOG(Log_Character_Player_Stat, Warning, TEXT("Affinity DataTable not loaded"));
	}

	BroadcastResourceStat(EResourceStatType::Health, PlayerStats.BaseStats.Health);
	BroadcastResourceStat(EResourceStatType::Stamina, PlayerStats.Stamina);
	BroadcastResourceStat(EResourceStatType::Focus, PlayerStats.Focus);
}

void UPlayerStatComponent::ChangeMaxStamina(const float Amount)
{
	PlayerStats.Stamina.Max += Amount;
	BroadcastResourceStat(EResourceStatType::Stamina, PlayerStats.Stamina);
}

bool UPlayerStatComponent::ChangeStamina(const float Amount, const EStatChangeType SPChangeType)
{
	float Delta = Amount;
	bool bChangeSuccess = false;

	switch (SPChangeType)
	{
	case EStatChangeType::Damage:
		bChangeSuccess = PlayerStats.Stamina.Current >= Delta;
		if (Delta > 0.f) TimeSinceStaminaSpend = 0.f;   // ← 핵심: 소모 시 회복 딜레이 리셋
		break;
	case EStatChangeType::Heal:
		bChangeSuccess = (PlayerStats.Stamina.Current + Delta) <= PlayerStats.Stamina.Max;
		Delta *= -1.0f;
		break;
	}

	PlayerStats.Stamina.Current = FMath::Clamp(PlayerStats.Stamina.Current - Delta, 0.0f, PlayerStats.Stamina.Max);

	BroadcastResourceStat(EResourceStatType::Stamina, PlayerStats.Stamina);

	return bChangeSuccess;
}

void UPlayerStatComponent::TickStaminaRegen(float DeltaTime)
{
	TimeSinceStaminaSpend += DeltaTime;

	if (PlayerStats.Stamina.Current >= PlayerStats.Stamina.Max) return;
	if (TimeSinceStaminaSpend < StaminaRegenDelay) return;

	// 기본 회복량 + 지구력 보너스
	const float EffectiveRegenRate = StaminaRegenRate + PlayerStats.CombatStats.StaminaRegenBonus;

	PlayerStats.Stamina.Current = FMath::Clamp(
		PlayerStats.Stamina.Current + EffectiveRegenRate * DeltaTime,
		0.f, PlayerStats.Stamina.Max);

	BroadcastResourceStat(EResourceStatType::Stamina, PlayerStats.Stamina);
}

float UPlayerStatComponent::GetAttributesRequirementRatio_Implementation(const FCharacterAttributes& RequireStats) const
{
	return BaseAttributes.GetRequirementAttributeRate(RequireStats);
}

float UPlayerStatComponent::GetWeaponPerformanceRatio_Implementation(const FCharacterAttributes& RequireStats) const
{
	const float StatFulfillRatio = BaseAttributes.GetRequirementAttributeRate(RequireStats);

	if (StatFulfillRatio >= 1.0f)
		return 1.0f;
	else if (StatFulfillRatio >= 0.8f)
		return 0.5f;
	else if (StatFulfillRatio >= 0.5f)
		return 0.3f;
	else
		return 0.2f;
}

void UPlayerStatComponent::ApplyArmorStats(float TotalDefense, float TotalMagicDefense, float TotalFireResistance, float TotalFrostResistance, float TotalPoisonResistance, float TotalBleedResistance, float TotalWeight)
{
	// 방어력
	PlayerStats.BaseStats.PhysicalDefense = TotalDefense;
	PlayerStats.BaseStats.MagicDefense = TotalMagicDefense;

	// 상태이상 저항력
	PlayerStats.BaseStats.FireResistance = TotalFireResistance;
	PlayerStats.BaseStats.FrostResistance = TotalFrostResistance;
	PlayerStats.BaseStats.PoisonResistance = TotalPoisonResistance;
	PlayerStats.BaseStats.BleedResistance = TotalBleedResistance;

	// 장비 하중 현재값 갱신
	// EquipLoad.Max는 Strength 특성으로 결정된 값 유지, Current만 교체
	PlayerStats.EquipLoad.Current = TotalWeight;
}