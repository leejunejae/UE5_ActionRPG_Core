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

    /** 일반형 적만 활성화한다. 엘리트와 보스는 비활성화한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Critical Execution")
        bool bCanBeCriticallyExecuted = true;

    /** 메시 Bounds 상단에서 체력바까지 추가로 띄울 높이. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (ClampMin = "0.0"))
        float HealthBarHeightOffset = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
        TMap<ELocomotionGait, FGaitSetting> LocomotionGaitData;
};
