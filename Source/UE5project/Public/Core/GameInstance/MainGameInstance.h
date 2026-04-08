// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"

/**
 * 
 */

class UWeaponDataAsset;

UCLASS()
class UE5PROJECT_API UMainGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
