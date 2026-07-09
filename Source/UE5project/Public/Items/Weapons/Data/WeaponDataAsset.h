// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "WeaponDataAsset.generated.h"

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

    // 히트 판정 반경(무기 고유)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float HitBoxRadius = 10.f;
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
        TSoftObjectPtr<UParticleSystem> Trail = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TSoftObjectPtr<USoundBase> Sound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FWeaponConfig WeaponConfig;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly)
        bool HasSubWeapon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "HasSubWeapon"))
        TSoftObjectPtr<UStaticMesh> SubMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "HasSubWeapon"))
        FWeaponConfig SubConfig;

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
        EWeaponType WeaponType; 

    // 리소스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
        FWeaponInstance WeaponInstance;
};
