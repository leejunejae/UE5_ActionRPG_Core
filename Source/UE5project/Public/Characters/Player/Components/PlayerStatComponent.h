#pragma once

#include "CoreMinimal.h"
#include "Characters/Components/StatComponent.h"
#include "Characters/Interfaces/StatInterface.h"

#include "PlayerStatComponent.generated.h"

UCLASS()
class UE5PROJECT_API UPlayerStatComponent : public UStatComponent,
	public IStatInterface
{
	GENERATED_BODY()

public:
	UPlayerStatComponent();

	void InitializeStats();

	FCharacterStats& GetCommonStats() override { return PlayerStats.BaseStats; }

	FORCEINLINE FPlayerStats GetCharacterStats_Native() const { return PlayerStats; }
	FPlayerStats GetCharacterStats_Implementation() const { return PlayerStats; }

#pragma region Attributes
public:
	FCharacterAttributes GetBaseAttributesLevel_Implementation() const { return BaseAttributes; }
	float GetAttributesRequirementRatio_Implementation(const FCharacterAttributes& RequireStats) const;
	float GetWeaponPerformanceRatio_Implementation(const FCharacterAttributes& RequireStats) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Stats")
	TMap<EAttributeType, UDataTable*> AttributeTables;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats") // 기본 특성값(힘, 민첩, 의지 등)
		FCharacterAttributes BaseAttributes;
#pragma endregion

#pragma region Stamina
public:
	bool ChangeStamina(const float Amount, const EStatChangeType SPChangeType);
	void ChangeMaxStamina(const float Amount);
	void TickStaminaRegen(float DeltaTime);

	FORCEINLINE float GetStamina() const { return PlayerStats.Stamina.Current; }
	FORCEINLINE float GetMaxStamina() const { return PlayerStats.Stamina.Max; }
	FORCEINLINE float GetStaminaRegenRate() const { return StaminaRegenRate; }

protected:
	UPROPERTY(EditAnywhere, Category = "Stats|Stamina")
	float StaminaRegenRate = 30.f;    // 초당 회복량

	UPROPERTY(EditAnywhere, Category = "Stats|Stamina")
	float StaminaRegenDelay = 1.0f;   // 소모 후 회복 시작까지 지연(초)

private:
	float TimeSinceStaminaSpend = 10.f;   // 시작 시 바로 회복 가능하도록 큰 값
#pragma endregion

#pragma region Equipment
public:
	// 방어구 전체 재계산 결과를 받아 방어력/저항력에 반영.
	// EquipmentComponent::RecalcArmorStats()에서 장착/해제마다 호출.
	// 무게는 더 이상 여기서 처리하지 않음 (ApplyEquipLoad로 분리).
	void ApplyArmorStats(
		float TotalDefense,
		float TotalMagicDefense,
		float TotalFireResistance,
		float TotalFrostResistance,
		float TotalPoisonResistance,
		float TotalBleedResistance);

	// 장비 하중 총합을 직접 반영.
	// EquipmentComponent가 무기+방어구 전체 무게를 계산해서 한 번에 전달.
	// EquipmentComponent::RecalcEquipLoad()에서 호출.
	void ApplyEquipLoad(float TotalLoad);
#pragma endregion

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats") // 특성값과 장비를 비롯한 요소로 결정되는 수치화된 캐릭터의 능력
		FPlayerStats PlayerStats;
};