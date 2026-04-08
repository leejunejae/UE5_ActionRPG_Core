// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/HitReactionData.h"

uint32 GetTypeHash(const FHitReactionDetail& HitReactionDetail)
{
	return GetTypeHash(HitReactionDetail.SectionName);
}

uint32 GetTypeHash(const FHitReactionInfo& Info)
{
	return GetTypeHash(Info.HitReactionName);
}