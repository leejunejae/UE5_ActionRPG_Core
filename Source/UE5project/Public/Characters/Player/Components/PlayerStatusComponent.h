// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Components/CharacterStatusComponent.h"
#include "PlayerStatusComponent.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API UPlayerStatusComponent : public UCharacterStatusComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPlayerStatusComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
