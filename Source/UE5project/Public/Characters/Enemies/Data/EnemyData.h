// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "Characters/Data/AffiliationData.h"
#include "Characters/Enemies/Data/EnemyComponentDataAsset.h"
#include "Characters/Enemies/Data/EnemyInstanceDataAsset.h"
#include "Characters/Enemies/Data/EnemyAIDataAsset.h"
#include "Core/MovementTypes.h"
#include "EnemyData.generated.h"

USTRUCT(BlueprintType)
struct FEnemyStats :public FTableRowBase
{
    GENERATED_BODY();

    /** 내부 식별용 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FName ID;

    /** 적 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FText Name;

    /** 적 위치 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FText Location;

    /** 체력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
        float Health;

    /** 강인도(강인도가 0이 되면 무력화 상태) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
        float Poise;

    /** 스탠스(스탠스가 0이 되면 경직 반응) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
        float Stance;

    /** 물리 방어력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
        float PhysicalDefense;

    /** 마법 방어력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
        float MagicDefense;

    /** 상태이상 저항 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
        float Resistance;

    /** 상태이상 저항 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
        float GuardNegation;

    /** 캐릭터 성향 : 공격성 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
        float Aggressiveness;

    /** 물리 공격력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attack", meta = (ClampMin = "0.0"))
        float PhysicalAttackPower = 0.f;

    /** 마법 공격력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attack", meta = (ClampMin = "0.0"))
        float MagicAttackPower = 0.f;

    /** 강인도 공격력 (상대 Poise 깎기) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attack", meta = (ClampMin = "0.0"))
        float PoiseAttackPower = 0.f;

    /** 스탠스 공격력 (상대 Stance 깎기) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attack", meta = (ClampMin = "0.0"))
        float StaminaAttackPower = 0.f;
};

USTRUCT(BlueprintType)
struct FEnemyInfo : public FTableRowBase
{
    GENERATED_BODY();

    /** 내부 식별용 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FName ID;

    /** 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FText Name;

    /** 위치 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FText Location;

    /** 세력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FGameplayTag Faction;

    /** 종족 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FGameplayTag Race;

    /** 역할 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        FGameplayTag Role;

    /** 드랍 테이블 */
   // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Refs")
       // TSoftObjectPtr<ULootTableDataAsset> LootTable;

    /** AI / 행동 세트 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        TSoftObjectPtr<UEnemyAIDataAsset> AIData;

    /** 메시/애니/사운드 등 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        TSoftObjectPtr<UEnemyInstanceDataAsset> InstanceData;
    
    /** 컴포넌트 데이터 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
        TSoftObjectPtr<UEnemyComponentDataAsset> CombatData;
};

// 참조, 캐싱, 반환을 위한 데이터 묶음
USTRUCT(BlueprintType)
struct FEnemyDataSet
{
    GENERATED_BODY();

public:
    const FEnemyInfo* Info;
    const FEnemyStats* Stats;

public:
    FEnemyDataSet() {}

    FEnemyDataSet(const FEnemyInfo* NewInfo, const FEnemyStats* NewStats)
        : Info(NewInfo), Stats(NewStats)
    {}
};

UCLASS()
class UE5PROJECT_API UEnemyData : public UObject
{
	GENERATED_BODY()

};
