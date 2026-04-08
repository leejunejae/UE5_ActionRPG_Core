// Copyright Epic Games, Inc. All Rights Reserved.


#include "Core/GameMode/UE5projectGameModeBase.h"
#include "Characters/Player/Warrior/FallenKnight.h"
#include "Characters/Player/Controller/ControllerBase.h"
#include "UI/DefaultWidget.h"

AUE5projectGameModeBase::AUE5projectGameModeBase()
{
	DefaultPawnClass = AFallenKnight::StaticClass();
	PlayerControllerClass = AControllerBase::StaticClass();
}

void AUE5projectGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}