// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ActionWindowRules.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UActionWindowRules : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows")
        TMap<FGameplayTag, FGameplayTagContainer> DefaultWindowsByState;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows")
        TMap<FGameplayTag, FGameplayTagContainer> CloseOnActionBegin;
};
