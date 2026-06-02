// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ControllerBase.generated.h"

/**
 * 
 */

class UPlayerHUDWidget;

UCLASS()
class UE5PROJECT_API AControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	/** HUD에 띄울 위젯 클래스 (블루프린트에서 WBP_PlayerHUD로 지정) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerHUDWidget> PlayerHUDClass;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void InitializeFullScreenUI();
	void CreatePlayerHUD();
	void BindHUDToPawn(APawn* InPawn);

	UPROPERTY()
	TObjectPtr<UPlayerHUDWidget> PlayerHUDWidget;
};
