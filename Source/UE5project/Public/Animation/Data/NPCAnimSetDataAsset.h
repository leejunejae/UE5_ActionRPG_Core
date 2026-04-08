// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/Data/NPCAnimProfileDataAsset.h"
#include "NPCAnimSetDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UNPCAnimSetDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TMap<FGameplayTag, TSoftObjectPtr<UNPCAnimProfileDataAsset>> NPCAnimSets;
};
