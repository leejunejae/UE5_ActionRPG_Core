// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Data/HitReactionData.h"
#include "Engine/DataAsset.h"
#include "HitReactionDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class UE5PROJECT_API UHitReactionDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<EHitResponse, FHitReactionInfo> HitReactionInfoList;

	FHitReactionInfo FindHitReactionInfo(const EHitResponse& Response, bool bLogNotFound = false) const;
};
