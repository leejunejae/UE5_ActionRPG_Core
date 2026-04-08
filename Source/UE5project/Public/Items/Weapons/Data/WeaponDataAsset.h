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
struct FWeaponInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UStaticMesh> WeaponMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UTexture2D> WeaponIcon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<UParticleSystem> WeaponTrail = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
        TObjectPtr<USoundBase> WeaponSound = nullptr;

public:
    bool IsValid() const
    {
        if (WeaponMesh) return true;
        else return false;
    }
};

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

UCLASS()
class UE5PROJECT_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FText DisplayName; // 표시용 이름

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        EWeaponType WeaponType; // 무기 유형

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FWeaponInstance WeaponInstance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FWeaponConfig WeaponConfig;

    UPROPERTY(EditAnyWhere, BlueprintReadWrite)
        bool HasSubWeapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "HasSubWeapon"))
        FWeaponInstance SubInstance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "HasSubWeapon"))
        FWeaponConfig SubConfig;
};
