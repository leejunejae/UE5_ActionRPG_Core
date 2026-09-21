// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "Items/Weapons/Data/WeaponAudioData.h"
#include "WeaponDataAsset.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

/**
 * 
 */

USTRUCT(BlueprintType)
struct FWeaponConfig
{
    GENERATED_BODY()

    // 무기 메시 피벗/손잡이 기준 차이를 해결하는 "무기 고유" 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector WeaponScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
    EWeaponTraceShape TraceShape = EWeaponTraceShape::Capsule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace|Capsule",
        meta = (EditCondition = "TraceShape == EWeaponTraceShape::Capsule"))
    FName TraceStartSocket = TEXT("Start");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace|Capsule",
        meta = (EditCondition = "TraceShape == EWeaponTraceShape::Capsule"))
    FName TraceEndSocket = TEXT("End");

    // 기존 에셋의 직렬화된 반경 값을 유지한다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace|Capsule",
        meta = (EditCondition = "TraceShape == EWeaponTraceShape::Capsule", ClampMin = "0.0"))
    float HitBoxRadius = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace|Box",
        meta = (EditCondition = "TraceShape == EWeaponTraceShape::Box"))
    FName TraceCenterSocket = TEXT("TraceCenter");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace|Box",
        meta = (EditCondition = "TraceShape == EWeaponTraceShape::Box", ClampMin = "0.0"))
    FVector BoxHalfExtent = FVector(8.0f, 30.0f, 45.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
    TSoftObjectPtr<UNiagaraSystem> TrailSystem = nullptr;

    // 비어 있으면 TrailSystem의 Ribbon Renderer 기본 머티리얼을 사용한다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
    TSoftObjectPtr<UMaterialInterface> TrailMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
    FName TrailStartSocket = TEXT("Start");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
    FName TrailEndSocket = TEXT("End");
};

USTRUCT(BlueprintType)
struct FWeaponInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<UStaticMesh> Mesh = nullptr;

        // 전체 아이콘
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UTexture2D> Icon = nullptr;

        // 슬롯용 아이콘
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UTexture2D> SlotIcon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FWeaponConfig WeaponConfig;

	// 비어 있으면 WeaponDataSubsystem의 무기 유형 기본 프로필을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<UWeaponAudioProfile> WeaponAudioProfileOverride = nullptr;

public:
    bool IsValid() const
    {
        return !Mesh.IsNull();
    }
};

/** 주무기와 보조무기가 공유하는 외형, 트레이스, Trail, 오디오 데이터. */
UCLASS(Abstract)
class UE5PROJECT_API UWeaponEquipmentDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    // 표시용 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
        FText DisplayName; 

    // 설명
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
        FText Description;
        
    // 물리 무기 계열. 애니메이션 및 장비 조합과 독립적이다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
        EWeaponCategory WeaponCategory = EWeaponCategory::None;

    // 리소스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
        FWeaponInstance WeaponInstance;

    EWeaponCategory GetEffectiveWeaponCategory() const
    {
        return WeaponCategory;
    }
};

/** 주무기 전용 정의. */
UCLASS()
class UE5PROJECT_API UWeaponDataAsset : public UWeaponEquipmentDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip")
    EWeaponGripType GripType = EWeaponGripType::OneHanded;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip",
        meta = (EditCondition = "GripType != EWeaponGripType::TwoHanded"))
    bool bCanEquipOffHand = false;
};

/** 보조무기 전용 정의. 모든 보조무기는 별도 행의 공격 스탯을 사용한다. */
UCLASS()
class UE5PROJECT_API UOffHandWeaponDataAsset : public UWeaponEquipmentDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip")
    FName DrawnSocket = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip")
    FName HolsterSocket = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip")
    FVector HolsterLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip")
    FRotator HolsterRotationOffset = FRotator::ZeroRotator;
};
