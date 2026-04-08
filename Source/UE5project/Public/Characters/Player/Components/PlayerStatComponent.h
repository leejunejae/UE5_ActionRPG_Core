// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Components/StatComponent.h"
#include "Characters/Interfaces/StatInterface.h"

#include "PlayerStatComponent.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UPlayerStatComponent : public UStatComponent,
	public IStatInterface
{
	GENERATED_BODY()
	
public:
	UPlayerStatComponent();

	void InitializeStats();

	FCharacterStats& GetCommonStats() override { return PlayerStats.BaseStats; }

	void ChangeMaxStamina(const float Amount);
	bool ChangeStamina(const float Amount, const EStatChangeType SPChangeType);

	FORCEINLINE FPlayerStats GetCharacterStats_Native() const { return PlayerStats; }

	FCharacterAttributes GetBaseAttributesLevel_Implementation() const { return BaseAttributes; }
	FPlayerStats GetCharacterStats_Implementation() const { return PlayerStats; }
	float GetAttributesRequirementRatio_Implementation(const FCharacterAttributes& RequireStats) const;
	float GetWeaponPerformanceRatio_Implementation(const FCharacterAttributes& RequireStats) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Stats")
		TMap<EAttributeType, UDataTable*> AttributeTables;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats") // 기본 특성값(힘, 민첩, 의지 등)
		FCharacterAttributes BaseAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats") // 특성값과 장비를 비롯한 요소로 결정되는 수치화된 캐릭터의 능력
		FPlayerStats PlayerStats;
};
