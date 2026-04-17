// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/Components/PlayerStatComponent.h"

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

	static ConstructorHelpers::FObjectFinder<UDataTable> IntelligenceDT_Asset(TEXT("DataTable'/Game/00_Character/Data/Stat/Attribute_Intelligence_DT.Attribute_Intelligence_DT'"));
	if (IntelligenceDT_Asset.Succeeded())
	{
		AttributeTables.Add(EAttributeType::Intelligence, IntelligenceDT_Asset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> VigorDT_Asset(TEXT("DataTable'/Game/00_Character/Data/Stat/Attribute_Vigor_DT.Attribute_Vigor_DT'"));
	if (VigorDT_Asset.Succeeded())
	{
		AttributeTables.Add(EAttributeType::Vigor, VigorDT_Asset.Object);
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
			UE_LOG(LogTemp, Warning, TEXT("Vitality is Unloaded"));
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
			UE_LOG(LogTemp, Warning, TEXT("Endurance is Unloaded"));
		}
	}

	if (AttributeTables.Contains(EAttributeType::Mentality))
	{
		FString RowName = FString::Printf(TEXT("Mentality_%d"), BaseAttributes.Mentality);
		const FAttribute_Mentality* MentalityDTRow = AttributeTables[EAttributeType::Mentality]->FindRow<FAttribute_Mentality>(FName(*RowName), TEXT(""));

		if (MentalityDTRow)
		{
			PlayerStats.Focus.Max = MentalityDTRow->FP;
			PlayerStats.Focus.Current = PlayerStats.Focus.Max;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Mentality is Unloaded"));
		}
	}

	if (AttributeTables.Contains(EAttributeType::Strength))
	{
		FString RowName = FString::Printf(TEXT("Strength_%d"), BaseAttributes.Strength);
		const FAttribute_Strength* StrengthDTRow = AttributeTables[EAttributeType::Strength]->FindRow<FAttribute_Strength>(FName(*RowName), TEXT(""));

		if (StrengthDTRow)
		{
			PlayerStats.BaseStats.PhysicalDefense = StrengthDTRow->PhysicalDefense;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Strength is Unloaded"));
		}
	}

	if (AttributeTables.Contains(EAttributeType::Dexterity))
	{
		FString RowName = FString::Printf(TEXT("Dexterity_%d"), BaseAttributes.Dexterity);
		const FAttribute_Dexterity* DexterityDTRow = AttributeTables[EAttributeType::Dexterity]->FindRow<FAttribute_Dexterity>(FName(*RowName), TEXT(""));

		if (DexterityDTRow)
		{
			PlayerStats.BaseStats.Poise.Max = DexterityDTRow->Poise;
			PlayerStats.BaseStats.Poise.Current = PlayerStats.BaseStats.Poise.Max;
			PlayerStats.Evasion = DexterityDTRow->Evasion;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Dexterity is Unloaded"));
		}
	}

	if (AttributeTables.Contains(EAttributeType::Intelligence))
	{
		FString RowName = FString::Printf(TEXT("Intelligence_%d"), BaseAttributes.Intelligence);
		const FAttribute_Intelligence* IntelligenceDTRow = AttributeTables[EAttributeType::Intelligence]->FindRow<FAttribute_Intelligence>(FName(*RowName), TEXT(""));

		if (IntelligenceDTRow)
		{
			PlayerStats.BaseStats.MagicDefense = IntelligenceDTRow->MagicDefense;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Intelligence is Unloaded"));
		}
	}

	if (AttributeTables.Contains(EAttributeType::Vigor))
	{
		FString RowName = FString::Printf(TEXT("Vigor_%d"), BaseAttributes.Vigor);
		const FAttribute_Vigor* VigorDTRow = AttributeTables[EAttributeType::Vigor]->FindRow<FAttribute_Vigor>(FName(*RowName), TEXT(""));

		if (VigorDTRow)
		{
			PlayerStats.EquipLoad.Max = VigorDTRow->EquipLoad;
			PlayerStats.BaseStats.Resistance = VigorDTRow->Resistance;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Vigor is Unloaded"));
		}
	}
}

void UPlayerStatComponent::ChangeMaxStamina(const float Amount)
{
	PlayerStats.Stamina.Max += Amount;
}

bool UPlayerStatComponent::ChangeStamina(const float Amount, const EStatChangeType SPChangeType)
{
	float Delta = Amount;
	bool bChangeSuccess = false;

	switch (SPChangeType)
	{
	case EStatChangeType::Damage:
		bChangeSuccess = PlayerStats.Stamina.Current >= Delta;
		break;
	case EStatChangeType::Heal:
		bChangeSuccess = (PlayerStats.Stamina.Current + Delta) <= PlayerStats.Stamina.Max;
		Delta *= -1.0f;
		break;
	}

	PlayerStats.Stamina.Current = FMath::Clamp(PlayerStats.Stamina.Current - Delta, 0.0f, PlayerStats.Stamina.Max);

	return bChangeSuccess;
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
