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

FPlayerStats UPlayerStatComponent::CalculateStatsForAttributes(const FCharacterAttributes& Attrs) const
{
    FPlayerStats Result;

    if (AttributeTables.Contains(EAttributeType::Vitality))
    {
        FString RowName = FString::Printf(TEXT("Vitality_%d"), Attrs.Vitality);
        if (const FAttribute_Vitality* Row = AttributeTables[EAttributeType::Vitality]->FindRow<FAttribute_Vitality>(FName(*RowName), TEXT("")))
        {
            Result.BaseStats.Health.Max = Row->HP;
        }
    }

    if (AttributeTables.Contains(EAttributeType::Endurance))
    {
        FString RowName = FString::Printf(TEXT("Endurance_%d"), Attrs.Endurance);
        if (const FAttribute_Endurance* Row = AttributeTables[EAttributeType::Endurance]->FindRow<FAttribute_Endurance>(FName(*RowName), TEXT("")))
        {
            Result.Stamina.Max = Row->Stamina;
        }
    }

    if (AttributeTables.Contains(EAttributeType::Mentality))
    {
        FString RowName = FString::Printf(TEXT("Mentality_%d"), Attrs.Mentality);
        if (const FAttribute_Mentality* Row = AttributeTables[EAttributeType::Mentality]->FindRow<FAttribute_Mentality>(FName(*RowName), TEXT("")))
        {
            Result.Focus.Max = Row->Focus;
        }
    }

    if (AttributeTables.Contains(EAttributeType::Strength))
    {
        FString RowName = FString::Printf(TEXT("Strength_%d"), Attrs.Strength);
        if (const FAttribute_Strength* Row = AttributeTables[EAttributeType::Strength]->FindRow<FAttribute_Strength>(FName(*RowName), TEXT("")))
        {
            Result.CombatStats.StrengthAttackBonus = Row->PhysicalAttackBonus;
            Result.BaseStats.Poise.InitResource(Row->PoiseMax);
            Result.EquipLoad.Max = Row->EquipLoadMax;
        }
    }

    if (AttributeTables.Contains(EAttributeType::Dexterity))
    {
        FString RowName = FString::Printf(TEXT("Dexterity_%d"), Attrs.Dexterity);
        if (const FAttribute_Dexterity* Row = AttributeTables[EAttributeType::Dexterity]->FindRow<FAttribute_Dexterity>(FName(*RowName), TEXT("")))
        {
            Result.CombatStats.DexterityAttackBonus = Row->PhysicalAttackBonus;
            Result.CombatStats.StaminaRegenBonus = Row->StaminaRegenBonus;
        }
    }

    // Affinity는 아직 DataTable 없음 - 추후 추가

    return Result;
}

void UPlayerStatComponent::InitializeStats()
{
    const FPlayerStats Calculated = CalculateStatsForAttributes(BaseAttributes);

    // 파생 스탯 max값 반영
    PlayerStats.BaseStats.Health.Max = Calculated.BaseStats.Health.Max;
    PlayerStats.BaseStats.Health.Current = PlayerStats.BaseStats.Health.Max;

    PlayerStats.Stamina.Max = Calculated.Stamina.Max;
    PlayerStats.Stamina.Current = PlayerStats.Stamina.Max;

    PlayerStats.Focus.Max = Calculated.Focus.Max;
    PlayerStats.Focus.Current = PlayerStats.Focus.Max;

    PlayerStats.BaseStats.Poise = Calculated.BaseStats.Poise;
    PlayerStats.EquipLoad.Max = Calculated.EquipLoad.Max;

    PlayerStats.CombatStats = Calculated.CombatStats;

    BroadcastResourceStat(EResourceStatType::Health, PlayerStats.BaseStats.Health);
    BroadcastResourceStat(EResourceStatType::Stamina, PlayerStats.Stamina);
    BroadcastResourceStat(EResourceStatType::Focus, PlayerStats.Focus);
}

FPlayerStats UPlayerStatComponent::PreviewStatsWithAttributeDelta(const TMap<EAttributeType, int32>& Deltas) const
{
    FCharacterAttributes PreviewAttrs = BaseAttributes;

    for (const auto& [Type, Delta] : Deltas)
    {
        switch (Type)
        {
        case EAttributeType::Vitality:   PreviewAttrs.Vitality = FMath::Clamp(PreviewAttrs.Vitality + Delta, 1, MaxAttributeValue); break;
        case EAttributeType::Endurance:  PreviewAttrs.Endurance = FMath::Clamp(PreviewAttrs.Endurance + Delta, 1, MaxAttributeValue); break;
        case EAttributeType::Mentality:  PreviewAttrs.Mentality = FMath::Clamp(PreviewAttrs.Mentality + Delta, 1, MaxAttributeValue); break;
        case EAttributeType::Strength:   PreviewAttrs.Strength = FMath::Clamp(PreviewAttrs.Strength + Delta, 1, MaxAttributeValue); break;
        case EAttributeType::Dexterity:  PreviewAttrs.Dexterity = FMath::Clamp(PreviewAttrs.Dexterity + Delta, 1, MaxAttributeValue); break;
        case EAttributeType::Affinity:   PreviewAttrs.Affinity = FMath::Clamp(PreviewAttrs.Affinity + Delta, 1, MaxAttributeValue); break;
        }
    }

    FPlayerStats Preview = CalculateStatsForAttributes(PreviewAttrs);

    // 현재값(Current)은 실제 PlayerStats 기준 유지 (미리보기는 Max만 의미 있음)
    Preview.BaseStats.Health.Current = PlayerStats.BaseStats.Health.Current;
    Preview.Stamina.Current = PlayerStats.Stamina.Current;
    Preview.Focus.Current = PlayerStats.Focus.Current;

    return Preview;
}

bool UPlayerStatComponent::CommitAttributeAllocation(const TMap<EAttributeType, int32>& Deltas)
{
    int32 TotalCost = 0;
    for (const auto& [Type, Delta] : Deltas)
    {
        TotalCost += Delta;
    }

    if (TotalCost <= 0 || TotalCost > AvailableStatPoints)
    {
        return false;
    }

    // 상한 체크
    FCharacterAttributes NewAttrs = BaseAttributes;
    for (const auto& [Type, Delta] : Deltas)
    {
        switch (Type)
        {
        case EAttributeType::Vitality:   NewAttrs.Vitality += Delta; break;
        case EAttributeType::Endurance:  NewAttrs.Endurance += Delta; break;
        case EAttributeType::Mentality:  NewAttrs.Mentality += Delta; break;
        case EAttributeType::Strength:   NewAttrs.Strength += Delta; break;
        case EAttributeType::Dexterity:  NewAttrs.Dexterity += Delta; break;
        case EAttributeType::Affinity:   NewAttrs.Affinity += Delta; break;
        }
    }

    if (NewAttrs.Vitality > MaxAttributeValue || NewAttrs.Endurance > MaxAttributeValue ||
        NewAttrs.Mentality > MaxAttributeValue || NewAttrs.Strength > MaxAttributeValue ||
        NewAttrs.Dexterity > MaxAttributeValue || NewAttrs.Affinity > MaxAttributeValue)
    {
        return false;
    }

    BaseAttributes = NewAttrs;
    AvailableStatPoints -= TotalCost;

    // 파생 스탯 재계산 + 현재값 증가분만큼 같이 올려주기 (예: HP Max 50 증가 시 Current도 50 증가)
    const FPlayerStats OldMax = PlayerStats; // 증가분 계산용
    const FPlayerStats NewCalculated = CalculateStatsForAttributes(BaseAttributes);

    const float HealthDelta = NewCalculated.BaseStats.Health.Max - OldMax.BaseStats.Health.Max;
    const float StaminaDelta = NewCalculated.Stamina.Max - OldMax.Stamina.Max;
    const float FocusDelta = NewCalculated.Focus.Max - OldMax.Focus.Max;

    PlayerStats.BaseStats.Health.Max = NewCalculated.BaseStats.Health.Max;
    PlayerStats.BaseStats.Health.Current = FMath::Clamp(PlayerStats.BaseStats.Health.Current + HealthDelta, 0.f, PlayerStats.BaseStats.Health.Max);

    PlayerStats.Stamina.Max = NewCalculated.Stamina.Max;
    PlayerStats.Stamina.Current = FMath::Clamp(PlayerStats.Stamina.Current + StaminaDelta, 0.f, PlayerStats.Stamina.Max);

    PlayerStats.Focus.Max = NewCalculated.Focus.Max;
    PlayerStats.Focus.Current = FMath::Clamp(PlayerStats.Focus.Current + FocusDelta, 0.f, PlayerStats.Focus.Max);

    PlayerStats.BaseStats.Poise = NewCalculated.BaseStats.Poise;
    PlayerStats.EquipLoad.Max = NewCalculated.EquipLoad.Max;
    PlayerStats.CombatStats = NewCalculated.CombatStats;

    BroadcastResourceStat(EResourceStatType::Health, PlayerStats.BaseStats.Health);
    BroadcastResourceStat(EResourceStatType::Stamina, PlayerStats.Stamina);
    BroadcastResourceStat(EResourceStatType::Focus, PlayerStats.Focus);

    return true;
}

#pragma region Attributes

float UPlayerStatComponent::GetAttributesRequirementRatio(const FCharacterAttributes& RequireStats) const
{
	return BaseAttributes.GetRequirementAttributeRate(RequireStats);
}

float UPlayerStatComponent::GetWeaponPerformanceRatio(const FCharacterAttributes& RequireStats) const
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

#pragma endregion


#pragma region Stamina

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
		if (Delta > 0.f) TimeSinceStaminaSpend = 0.f;   // 핵심: 소모 시 회복 딜레이 리셋
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

#pragma endregion


#pragma region Equipment

void UPlayerStatComponent::ApplyArmorStats(
	float TotalDefense,
	float TotalMagicDefense,
	float TotalFireResistance,
	float TotalFrostResistance,
	float TotalPoisonResistance,
	float TotalBleedResistance)
{
	// 방어력
	PlayerStats.BaseStats.PhysicalDefense = TotalDefense;
	PlayerStats.BaseStats.MagicDefense = TotalMagicDefense;

	// 상태이상 저항력
	PlayerStats.BaseStats.FireResistance = TotalFireResistance;
	PlayerStats.BaseStats.FrostResistance = TotalFrostResistance;
	PlayerStats.BaseStats.PoisonResistance = TotalPoisonResistance;
	PlayerStats.BaseStats.BleedResistance = TotalBleedResistance;
}

void UPlayerStatComponent::ApplyEquipLoad(float TotalLoad)
{
	// EquipLoad.Max는 Strength 특성으로 결정된 값 유지, Current만 갱신
	PlayerStats.EquipLoad.Current = TotalLoad;
}

#pragma endregion