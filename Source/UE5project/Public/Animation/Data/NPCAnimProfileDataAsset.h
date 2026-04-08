// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/Data/AnimData.h"
#include "NPCAnimProfileDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UNPCAnimProfileDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
		TMap<FGameplayTag, FAnimDataSet> AnimProfiles;
};
