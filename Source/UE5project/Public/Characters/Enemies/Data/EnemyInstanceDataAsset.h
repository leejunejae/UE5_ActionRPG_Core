// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/Data/AnimData.h"
#include "Items/Weapons/Data/WeaponDataAsset.h"
#include "Core/MovementTypes.h"
#include "EnemyInstanceDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UEnemyInstanceDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    /** 캐릭터 외형 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
        TObjectPtr<USkeletalMesh> SkeletalMesh;

    /** 애니메이션 블루프린트 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
        TSubclassOf<UAnimInstance> AnimBlueprint;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
        FGameplayTag SkeletonTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
        TObjectPtr<UWeaponDataAsset> WeaponData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
        FGameplayTag WeaponTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
        bool UseDefaultAnim; // 기본(Unarmed)상태의 애니메이션 사용여부

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
        TMap<ELocomotionGait, FGaitSetting> LocomotionGaitData;
};
