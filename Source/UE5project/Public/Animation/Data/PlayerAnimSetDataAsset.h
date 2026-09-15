// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/Weapons/Data/WeaponData.h"
#include "Animation/Data/AnimData.h"
#include "PlayerAnimSetDataAsset.generated.h"

/**
 * 
 */

/** 플레이어의 물리 장비 조합을 실행할 전투 프로필 태그로 변환한다. */
USTRUCT(BlueprintType)
struct FPlayerCombatStyleRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EWeaponCategory MainWeaponCategory = EWeaponCategory::None;

    // None은 해당 파지 방식의 주무기 기본 규칙을 뜻한다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EWeaponCategory OffHandWeaponCategory = EWeaponCategory::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EWeaponGripMode GripMode = EWeaponGripMode::OneHanded;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "CombatStyle"))
    FGameplayTag CombatStyle;
};

UCLASS()
class UE5PROJECT_API UPlayerAnimSetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    // 무기와 무관한 기본 애니메이션. AnimList의 비어 있지 않은 필드만 이를 덮어쓴다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Common")
        FPlayerAnimSet CommonAnimSet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TMap<EWeaponType, FPlayerAnimSet> AnimList;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Combat Style",
        meta = (Categories = "CombatStyle"))
        TMap<FGameplayTag, FPlayerAnimSet> CombatStyleAnimList;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat Style")
        TArray<FPlayerCombatStyleRule> CombatStyleRules;

   const FPlayerAnimSet* FindPlayerAnimSet(const EWeaponType& WeaponType, bool bLogNotFound = false) const;
   FPlayerAnimSet ResolvePlayerAnimSet(const EWeaponType& WeaponType) const;
   const FPlayerAnimSet* FindPlayerAnimSet(FGameplayTag CombatStyle, bool bLogNotFound = false) const;
   FPlayerAnimSet ResolvePlayerAnimSet(FGameplayTag CombatStyle) const;
   FGameplayTag ResolveCombatStyle(EWeaponCategory MainWeaponCategory,
       EWeaponCategory OffHandWeaponCategory, EWeaponGripMode GripMode) const;
};
