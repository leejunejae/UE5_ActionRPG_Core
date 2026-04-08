// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Data/AttackData.h"

uint32 GetTypeHash(const FAttackContext& AttackInfo)
{
	return GetTypeHash(AttackInfo.AttackName);
}