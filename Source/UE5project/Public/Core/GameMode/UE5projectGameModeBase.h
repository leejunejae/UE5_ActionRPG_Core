// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UE5projectGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class UE5PROJECT_API AUE5projectGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AUE5projectGameModeBase();

	virtual void PostLogin(APlayerController* NewPlayer) override;
};
