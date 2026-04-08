// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTags.h"
#include "AnimBoneDataRegistryRoot.generated.h"

/**
 * 
 */

class UAttackBoneDataRegistry;

UCLASS()
class UE5PROJECT_API UAnimBoneDataRegistryRoot : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TMap<FGameplayTag, TSoftObjectPtr<UAttackBoneDataRegistry>> AnimBoneDataRegistry;
};
