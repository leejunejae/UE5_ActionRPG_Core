// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/DataAsset/HitReactionDataAsset.h"

FHitReactionInfo UHitReactionDataAsset::FindHitReactionInfo(const EHitResponse& Response, bool bLogNotFound) const
{
	const FHitReactionInfo* Info = HitReactionInfoList.Find(Response);
	if (Info)
	{
		return *Info;
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Not HitReaction Info"))
	}

	return FHitReactionInfo();
}