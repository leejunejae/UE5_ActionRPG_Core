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

    // 기존 검+방패 묶음 에셋의 로딩 호환용. 새 에셋은 독립 OffHand 장비를 사용한다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DeprecatedProperty, DeprecationMessage = "Use an independent OffHand weapon asset."))
        bool HasSubWeapon = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "HasSubWeapon"))
        TSoftObjectPtr<UStaticMesh> SubMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "HasSubWeapon"))
        FWeaponConfig SubConfig;

	// 비어 있으면 WeaponDataSubsystem의 무기 유형 기본 프로필을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<UWeaponAudioProfile> WeaponAudioProfileOverride = nullptr;

public:
    bool IsValid() const
    {
        return !Mesh.IsNull();
    }
};

UCLASS()
class UE5PROJECT_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    // 표시용 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
        FText DisplayName; 

    // 설명
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
        FText Description;
        
    // 유형
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Legacy", meta = (DeprecatedProperty, DeprecationMessage = "Use WeaponCategory and CombatStyle."))
        EWeaponType WeaponType = EWeaponType::None;

    // 물리 무기 계열. 애니메이션 및 장비 조합과 독립적이다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
        EWeaponCategory WeaponCategory = EWeaponCategory::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip")
        EWeaponGripType GripType = EWeaponGripType::OneHanded;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip")
        EEquipmentHandSlot AllowedSlot = EEquipmentHandSlot::MainHand;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Equip", meta = (EditCondition = "GripType != EWeaponGripType::TwoHanded"))
        bool bCanEquipOffHand = false;

    // 리소스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
        FWeaponInstance WeaponInstance;

    EWeaponCategory GetEffectiveWeaponCategory() const
    {
        return WeaponCategory != EWeaponCategory::None
            ? WeaponCategory
            : GetWeaponCategoryFromLegacyType(WeaponType);
    }
};
